#include "simulator.h"
#include "util/glmExtraOps.h"
#include <algorithm>
#include <omp.h>
#include "util/paralellDefine.h"
#include <stdexcept>
#include <chrono>
#include "util/compTimeForLoop.h"
#include "util/random.h"
#include "util/interpolation.h"
#include <mutex>

#define _USE_MATH_DEFINES
#include <math.h>

using namespace genericfsim::macgrid;
using namespace genericfsim::particles;
using namespace genericfsim::simulator;
using namespace genericfsim::util;
using namespace genericfsim::obstacle;

constexpr bool PARALLEL_SIM_PART		= RUN_IN_PARALLEL;
constexpr bool PARALLEL_PUSH_APART		= RUN_IN_PARALLEL;
constexpr bool PARALLEL_PUSH_OUT		= RUN_IN_PARALLEL;
constexpr bool PARALLEL_P2G				= RUN_IN_PARALLEL;
constexpr bool PARALLEL_INCOMPR			= RUN_IN_PARALLEL;
constexpr bool PARALLEL_INCOMPR_PREP	= RUN_IN_PARALLEL;
constexpr bool PARALLEL_G2P				= RUN_IN_PARALLEL;


Simulator::Simulator(SimulatorConfig config, std::shared_ptr<HashedParticles> hashedParticles, std::shared_ptr<MacGrid> macGrid)
	: config(std::move(config)), hashedParticles(hashedParticles), macGrid(macGrid) {
	stepDuration["SimulateParticles"] = 0;
	stepDuration["PushParticlesApart"] = 0;
	stepDuration["PushParticlesOutOfObstacles"] = 0;
	stepDuration["P2GTransfer"] = 0;
	stepDuration["IncompressibilityPrep"] = 0;
	stepDuration["Incompressibility"] = 0;
	stepDuration["VelocityExtrapolation"] = 0;
	stepDuration["G2PTransfer"] = 0;
}

void Simulator::setNewMacGrid(std::shared_ptr<MacGrid> macGrid) {
	this->macGrid = macGrid;
}

void genericfsim::simulator::Simulator::setNewHashedParticles(std::shared_ptr<genericfsim::particles::HashedParticles> particles) {
	this->hashedParticles = particles;
}

void Simulator::simulate(double dt) {
	constexpr double slidingAvgFactor = 0.9;

	auto start = std::chrono::high_resolution_clock::now();
	if(config.particleSpawningEnabled)
		spawnParticles(dt);
	advectParticles(PARALLEL_SIM_PART, dt);
	stepDuration["SimulateParticles"] = stepDuration["SimulateParticles"] * slidingAvgFactor + std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * (1.0 - slidingAvgFactor);

	start = std::chrono::high_resolution_clock::now();
	if (config.pushApartEnabled) {
		hashedParticles->updateParticleIntersectionHash(PARALLEL_PUSH_APART);
		hashedParticles->pushParticlesApart(PARALLEL_PUSH_APART);
	}
	stepDuration["PushParticlesApart"] = stepDuration["PushParticlesApart"] * slidingAvgFactor + std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * (1.0 - slidingAvgFactor);

	start = std::chrono::high_resolution_clock::now();
	pushParticlesOutOfObstacles(PARALLEL_PUSH_OUT);
	stepDuration["PushParticlesOutOfObstacles"] = stepDuration["PushParticlesOutOfObstacles"] * slidingAvgFactor + std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * (1.0 - slidingAvgFactor);

	if(config.stopParticles)
		hashedParticles->forEach(PARALLEL_SIM_PART, [&](Particle& particle, int) {
			particle.v = glm::dvec3(0.0);
		});

	start = std::chrono::high_resolution_clock::now();
	macGrid->resetGridValues(PARALLEL_P2G);
	p2gTransfer(PARALLEL_P2G, dt);
	stepDuration["P2GTransfer"] = stepDuration["P2GTransfer"] * slidingAvgFactor + std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * (1.0 - slidingAvgFactor);

	start = std::chrono::high_resolution_clock::now();
	markFluidCellsAndCalculateParticleDensities(PARALLEL_INCOMPR_PREP);
	addObstaclesToGrid(PARALLEL_INCOMPR_PREP);
	macGrid->restoreBorderingSolidCellsAndSpeeds(PARALLEL_INCOMPR_PREP);
	macGrid->postP2GUpdate(PARALLEL_INCOMPR_PREP, config.gravityEnabled ? config.gravity * dt : 0.0);
	stepDuration["IncompressibilityPrep"] = stepDuration["IncompressibilityPrep"] * slidingAvgFactor + std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * (1.0 - slidingAvgFactor);

	start = std::chrono::high_resolution_clock::now();
	int itCount = macGrid->solveIncompressibility(PARALLEL_INCOMPR, dt);
	stepDuration["Incompressibility"] = stepDuration["Incompressibility"] * slidingAvgFactor + std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * (1.0 - slidingAvgFactor);
	stepDuration["Incompressibility it count"] = itCount;

	start = std::chrono::high_resolution_clock::now();
	macGrid->extrapolateVelocities(PARALLEL_G2P);
	stepDuration["VelocityExtrapolation"] = stepDuration["VelocityExtrapolation"] * slidingAvgFactor + std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * (1.0 - slidingAvgFactor);

	start = std::chrono::high_resolution_clock::now();
	g2pTransfer(PARALLEL_G2P);
	stepDuration["G2PTransfer"] = stepDuration["G2PTransfer"] * slidingAvgFactor + std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() * (1.0 - slidingAvgFactor);
}

std::map<std::string, long long> Simulator::getStepDuration() const {
	return stepDuration;
}

void Simulator::spawnParticles(double dt) {
	std::vector<Particle> newParticles;
	for (auto& obstacle : obstacles) {
		if (SphericalParticleSource* tmp = dynamic_cast<SphericalParticleSource*>(obstacle.get()); tmp != nullptr) {
			SphericalParticleSource& obstacle = *tmp;
			double r = obstacle.r + hashedParticles->getParticleR();
			double particleNumD = obstacle.particleSpawnRate * dt + obstacle.lastSpawnFraction;
			int particleNum = particleNumD;
			obstacle.lastSpawnFraction = particleNumD - particleNum;
			for (int i = 0; i < particleNum; i++) {
				double theta = genericfsim::util::getDoubleInRange(0.0, 2.0 * M_PI);
				double phi = genericfsim::util::getDoubleInRange(0.0, M_PI);
				glm::dvec3 normal(r * sin(phi) * cos(theta), r * sin(phi) * sin(theta), r * cos(phi));
				glm::dvec3 pos = obstacle.pos + normal;
				newParticles.push_back(Particle(pos, obstacle.particleSpawnSpeed * glm::normalize(normal)));
			}
		}
	}
	hashedParticles->addParticles(std::move(newParticles));
}

bool isParticleInRectangle(const glm::dvec3& pPos, double particleR, const glm::dvec3& rPos, const glm::dvec3& rSize) {
	glm::dvec3 d = (pPos - rPos);
	return (std::abs(d.x) < rSize.x * 0.5 + particleR) &&
		(std::abs(d.y) < rSize.y * 0.5 + particleR) &&
		(std::abs(d.z) < rSize.z * 0.5 + particleR);
}

void Simulator::advectParticles(bool parallel, double dt) {
	constexpr double wallRestitution = 0.3;
	constexpr double sphereRestitution = 1.0;
	constexpr double rectangleRestitution = 0.2;
	const glm::dvec3 cellD = macGrid->cellD;
	const double particleR = hashedParticles->getParticleR();
	const glm::dvec3 gridLow = cellD + glm::dvec3(particleR, particleR, macGrid->twoD ? 0.0 : particleR) * 1.01;
	const glm::dvec3 gridHigh = macGrid->dimensions - gridLow;

	std::vector<int> particleIdsToRemove;
	std::mutex particleIdsToRemoveMutex;

	hashedParticles->forEach(parallel, [&](Particle& particle, int idx) {
		double t = 0;
		int run = 0;
		const int maxRunCount = 200;
		while (run < maxRunCount) {
			run++;

			double minTBeforeCollision = 1e6;
			int minAxis = 0;

			COMP_FOR_LOOP(axis, 3,
				double component = particle.v[axis];
				double tmp = 1e6;
				if (component > 1e-6) {
					tmp = (gridHigh[axis] - particle.pos[axis]) / component;
				}
				else if (component < -1e-6) {
					tmp = (particle.pos[axis] - gridLow[axis]) / -component;
				}
				if (tmp < minTBeforeCollision) {
					minTBeforeCollision = tmp;
					minAxis = axis;
				}
			)

			if (minTBeforeCollision <= (dt - t)) {
				particle.pos += particle.v * minTBeforeCollision * 0.999;
				particle.v[minAxis] *= -wallRestitution;
				t += 0.999 * minTBeforeCollision;
				continue;
			}

			bool collision = false;
			particle.pos += particle.v * (dt - t);
			for (auto& obstacle : obstacles) {
				if (const SphericalObstacle* tmp = dynamic_cast<const SphericalObstacle*>(obstacle.get()); tmp != nullptr) {
					bool isSink = dynamic_cast<const SphericalParticleSink*>(obstacle.get()) != nullptr;
					const SphericalObstacle& obstacle = *tmp;
					double r = obstacle.r + particleR;
					double d = glm::length(obstacle.pos - particle.pos) - r;
					if (d < 0) {
						if (isSink && config.particleDespawningEnabled) {
							std::scoped_lock lock(particleIdsToRemoveMutex);
							particleIdsToRemove.push_back(idx);
							collision = false;
							break;
						}
						double backTime = 0;
						while (glm::length(particle.pos - particle.v * backTime - (obstacle.pos - obstacle.speed * backTime)) < r && backTime < t + 0.001f)
							backTime += 0.0002;
						if (backTime > dt - t)
							continue;
						backTime += 0.0002;
						particle.pos -= backTime * particle.v;
						glm::dvec3 posTmp = obstacle.pos - obstacle.speed * backTime;
						glm::dvec3 normal = glm::normalize(particle.pos - posTmp);
						glm::dvec3 relativeV = particle.v - obstacle.speed;
						double speedSemiNormalizer = -glm::dot(normal, relativeV);
						if (speedSemiNormalizer <= 0.0f)
							continue;
						glm::dvec3 speedMirror = -relativeV / speedSemiNormalizer;
						particle.v = (normal - speedMirror) * 2.0 + speedMirror;
						particle.v *= speedSemiNormalizer * sphereRestitution;
						particle.v += obstacle.speed;
						t += (dt - t) - backTime;
						collision = true;
						break;
					}
				}
				if (const RectengularObstacle* tmp = dynamic_cast<const RectengularObstacle*>(obstacle.get()); tmp != nullptr) {
					const RectengularObstacle& obstacle = *tmp;
					if (isParticleInRectangle(particle.pos, particleR, obstacle.pos, obstacle.size)) {
						double backTime = 0;
						while (isParticleInRectangle(particle.pos - particle.v * backTime, particleR, obstacle.pos - obstacle.speed * backTime, obstacle.size) && backTime < dt - t)
							backTime += 0.0002;
						if (backTime > dt - t)
							continue;
						backTime += 0.0002;
						particle.pos -= backTime * particle.v;
						glm::dvec3 posTmp = obstacle.pos - obstacle.speed * backTime;
						for (int axis = 0; axis < 3; axis++) {
							if (particle.pos[axis] >= posTmp[axis] + obstacle.size[axis] * 0.5 + particleR) {
								particle.v[axis] = -particle.v[axis] * rectangleRestitution + obstacle.speed[axis];
								break;
							}
							else if (particle.pos[axis] <= posTmp[axis] - obstacle.size[axis] * 0.5 - particleR) {
								particle.v[axis] = -particle.v[axis] * rectangleRestitution - obstacle.speed[axis];
								break;
							}
						}
						t += (dt - t) - backTime;
						collision = true;
						break;
					}
				}
			}
			if (!collision)
				run = maxRunCount;
		}
		COMP_FOR_LOOP(axis, 3,
			particle.pos[axis] = std::clamp(particle.pos[axis], gridLow[axis], gridHigh[axis]);
		)
	});

	hashedParticles->removeParticles(std::move(particleIdsToRemove));
}

void Simulator::pushParticlesOutOfObstacles(bool parallel) {
	const double particleR = hashedParticles->getParticleR();
	const glm::dvec3 cellD = macGrid->cellD;
	const glm::dvec3 dimensions = macGrid->dimensions;
	const glm::dvec3 particleLow(cellD.x + particleR * 1.01, cellD.y + particleR * 1.01, cellD.z + particleR * 1.01);
	const glm::dvec3 particleHigh = dimensions - particleLow;
	const bool zConst = hashedParticles->zConst;
	const double zConstVal = hashedParticles->z;

	for (auto& obstacle : obstacles) {
		if (const SphericalObstacle* tmp = dynamic_cast<const SphericalObstacle*>(obstacle.get()); tmp != nullptr) {
			const SphericalObstacle& obstacle = *tmp;
			double r = obstacle.r + particleR;
			double r2 = r * r;
			hashedParticles->forEach(parallel, [&](Particle& particle, int) {
				glm::dvec3 o2p = particle.pos - obstacle.pos;
				double distance2 = glm::dot(o2p, o2p);
				if (distance2 >= r2)
					return;
				double distance = sqrt(distance2);
				particle.pos = obstacle.pos + o2p / distance * r;
				particle.pos.x = std::clamp(particle.pos.x, particleLow.x, particleHigh.x);
				particle.pos.y = std::clamp(particle.pos.y, particleLow.y, particleHigh.y);
				particle.pos.z = zConst ? zConstVal : std::clamp(particle.pos.z, particleLow.z, particleHigh.z);
			});
		}
		if (const RectengularObstacle* tmp = dynamic_cast<const RectengularObstacle*>(obstacle.get()); tmp != nullptr) {
			const RectengularObstacle& obstacle = *tmp;
			auto [start, end] = macGrid->getMinMaxRect(obstacle.pos, obstacle.size);
			start -= glm::dvec3(particleR, particleR, particleR);
			end += glm::dvec3(particleR, particleR, particleR);
			hashedParticles->forEach(parallel, [&](Particle& particle, int) {
				if ((particle.pos.x > end.x || particle.pos.x < start.x) || (particle.pos.y > end.y || particle.pos.y < start.y)
					|| ((particle.pos.z > end.z || particle.pos.z < start.z) && !zConst))
					return;
				int axis = 0;
				double amount = 1e6;
				int axisNum = zConst ? 2 : 3;
				for (int p = 0; p < axisNum; p++) {
					if (particle.pos[p] > obstacle.pos[p]) {
						double tmp = end[p] - particle.pos[p];
						if (tmp < std::abs(amount)) {
							amount = tmp;
							axis = p;
						}
					}
					else {
						double tmp = start[p] - particle.pos[p];
						if (-tmp < std::abs(amount)) {
							amount = tmp;
							axis = p;
						}
					}
				}
				particle.pos[axis] += amount;
				particle.pos[axis] = std::clamp(particle.pos[axis], particleLow[axis], particleHigh[axis]);
			});
		}
	}
}

void Simulator::p2gTransfer(bool parallel, double dt) {
	const glm::dvec3 cellDInv = macGrid->cellDInv;

	hashedParticles->forEach(parallel, [&](Particle& particle, int) {
		auto faces = macGrid->getFacesAround(particle.pos);
		COMP_FOR_LOOP(axis, 3,
			COMP_FOR_LOOP(p, 8,
				MacGridCell::Face& face = faces[axis][p].face;
				double weight = trilinearInterpoll(face.pos, particle.pos, cellDInv);
				if (config.transferType == P2G2PType::PIC || config.transferType == P2G2PType::FLIP) {
					face.v += particle.v[axis] * weight;
				}
				else if (config.transferType == P2G2PType::APIC) {
					glm::dvec3 p2f = face.pos - particle.pos;
					face.v += (particle.v[axis] + glm::dot(particle.c[axis], p2f)) * weight;
				}
				face.particleWeightSum += weight;
			)
		)
	});

	macGrid->forEachCell(parallel, true, [&](glm::ivec3, MacGridCell& cell) {
		double w0 = cell.faces[0].particleWeightSum.load();
		double w1 = cell.faces[1].particleWeightSum.load();
		double w2 = cell.faces[2].particleWeightSum.load();
		if (w0 > 1e-6)
			cell.faces[0].v = cell.faces[0].v / w0;
		else
			cell.faces[0].v = 0.0;
		if (w1 > 1e-6)
			cell.faces[1].v = cell.faces[1].v / w1;
		else
			cell.faces[1].v = 0.0;
		if (w2 > 1e-6)
			cell.faces[2].v = cell.faces[2].v / w2;
		else
			cell.faces[2].v = 0.0;
	});
}

void Simulator::markFluidCellsAndCalculateParticleDensities(bool parallel) {
	const glm::dvec3 cellDInv = macGrid->cellDInv;

	hashedParticles->forEach(parallel, [&](Particle& particle, int) {
		glm::dvec3 pos = particle.pos * cellDInv;
		MacGridCell& cell = macGrid->cell(pos);
		cell.type = MacGridCell::CellType::WATER;

		auto cells = macGrid->getCellsAround(particle.pos);
		COMP_FOR_LOOP(p, 8,
			double weight = trilinearInterpoll(cells[p].cell.pos, particle.pos, cellDInv);
			cells[p].cell.avgPNum += weight;
		)
	});
}

void Simulator::addObstaclesToGrid(bool parallel) {
	for (auto& o : obstacles)
		macGrid->addObstacle(parallel, o.get());
}

void Simulator::g2pTransfer(bool parallel) {
	const glm::dvec3 cellDInv = macGrid->cellDInv;
	const bool twoD = macGrid->twoD;

	hashedParticles->forEach(parallel, [&](Particle& particle, int) {
		auto faces = macGrid->getFacesAround(particle.pos);
		for (int axis = 0; axis < 3; axis++) {
			if (twoD && axis == 2) {
				particle.v.z = 0;
				break;
			}

			double picComponent = 0;
			double flipComponent = 0;
			glm::dvec3 cvec(0, 0, 0);
			
			for (int p = 0; p < 8; p++) {
				const MacGridCell::Face& face = faces[axis][p].face;

				double weight = trilinearInterpoll(face.pos, particle.pos, cellDInv);
				picComponent += face.v2 * weight;
				
				if(config.transferType == P2G2PType::FLIP)
					flipComponent += (face.v2 - face.v) * weight;

				if(config.transferType == P2G2PType::APIC)
					cvec += trilinearInterpollGradient(face.pos, particle.pos, cellDInv) * face.v2;			//TODO: faceCenter és particle.pos sorrend jó?????
			}
			switch (config.transferType) {
			case P2G2PType::PIC:
				particle.v[axis] = picComponent;
				break;
			case P2G2PType::FLIP:
				particle.v[axis] = picComponent * (1 - config.flipRatio) 
					+ (flipComponent + particle.v[axis]) * config.flipRatio;
				break;
			case P2G2PType::APIC:
				particle.v[axis] = picComponent;
				particle.c[axis] = cvec;
				break;
			}
		}
	});
}
