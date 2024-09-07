#include "hashedParticles.h"
#include "../util/random.h"
#include "../util/glmExtraOps.h"
#include <omp.h>
#include <algorithm>

using namespace genericfsim::particles;

inline glm::ivec3 getCellCoord(const glm::dvec3& pos, double dInv) {
	return glm::ivec3(pos.x * dInv, pos.y * dInv, pos.z * dInv);
}

inline glm::ivec3 getCellMinCoord(const glm::dvec3& pos, const glm::dvec3& cellDInv, int axis) {
	glm::dvec3 coordOnGrid = pos * cellDInv - glm::dvec3(0.5, 0.5, 0.5);
	if(axis != 3)
		coordOnGrid[axis] += 0.5;
	glm::ivec3 idx(coordOnGrid.x, coordOnGrid.y, coordOnGrid.z);
	if(axis != 3)
		idx[axis] -= 1;
	return idx;
}

HashedParticles::HashedParticles(int num, double r, glm::dvec3 dimensions, glm::dvec3 cellD, bool zConst, double z) 
	: dimensions(std::move(dimensions)), cellD(std::move(cellD)), cellDInv(1.0 / cellD), r(r), z(z), zConst(zConst) {
	particles.reserve(num);
	for (int p = 0; p < num; p++) {
		Particle particle;
		particle.pos = glm::dvec3(
			util::getDoubleInRange(dimensions.x * 0.5 + cellD.x, dimensions.x - 1.1 * r - cellD.x),
			util::getDoubleInRange(dimensions.y * 0.5 + cellD.y, dimensions.y - 1.1 * r - cellD.y),
			zConst ? z : util::getDoubleInRange(dimensions.z * 0.5 + cellD.z, dimensions.z - 1.1 * r - cellD.z));
		particle.c[0] = particle.c[1] = particle.c[2] = glm::dvec3(0, 0, 0);
		particle.v = glm::dvec3(0, 0, 0);
		particles.push_back(std::move(particle));
	}
	particleIds = std::vector<int>(num);
	particleFaceIds[0] = std::vector<int>(num);
	particleFaceIds[1] = std::vector<int>(num);
	particleFaceIds[2] = std::vector<int>(num);
	setParticleR(r);
	setParticleNum(num);
}

void HashedParticles::forEachIntersecting(const Particle& particle, int pIndex, std::function<void(Particle&)>&& lambda) {
	glm::ivec3 cellIndex = getCellCoord(particle.pos, dInv);
	glm::ivec3 indexMax(std::min(cellIndex.x + 1, cellNum.x - 1), std::min(cellIndex.y + 1, cellNum.y - 1), std::min(cellIndex.z + 1, cellNum.z - 1));

	for (int x = std::max(cellIndex.x - 1, 0); x <= indexMax.x; x++) {
		for (int y = std::max(cellIndex.y - 1, 0); y <= indexMax.y; y++) {
			for (int z = std::max(cellIndex.z - 1, 0); z <= indexMax.z; z++) {
				int cellIndex = x * cellNum.y * cellNum.z + y * cellNum.z + z;
				int cellStartPos = particleCells[cellIndex];
				int maxPos = particleCells[cellIndex + 1];
				for (int idIndex = cellStartPos; idIndex < maxPos; idIndex++) {
					if (particleIds[idIndex] == pIndex)
						continue;
					lambda(particles[particleIds[idIndex]]);
				}
			}
		}
	}
}

void HashedParticles::pushParticlesApart(bool parallel) {
	const double particleD = r * 2;
	const double particleD2 = particleD * particleD;
	const glm::dvec3 particleLow(cellD.x + r * 1.01, cellD.y + r * 1.01, cellD.z + r * 1.01);
	const glm::dvec3 particleHigh = dimensions - particleLow;
	const double zConstVal = z;

	forEach(parallel, [&](Particle& particle, int idx) {
		glm::ivec3 cellIndex = getCellCoord(particle.pos, dInv);
		glm::ivec3 indexMax(std::min(cellIndex.x + 1, cellNum.x - 1), std::min(cellIndex.y + 1, cellNum.y - 1), std::min(cellIndex.z + 1, cellNum.z - 1));

		for (int x = std::max(cellIndex.x - 1, 0); x <= indexMax.x; x++) {
			for (int y = std::max(cellIndex.y - 1, 0); y <= indexMax.y; y++) {
				for (int z = std::max(cellIndex.z - 1, 0); z <= indexMax.z; z++) {
					int cellIndex = x * cellNum.y * cellNum.z + y * cellNum.z + z;
					int cellStartPos = particleCells[cellIndex];
					int maxPos = particleCells[cellIndex + 1];
					for (int idIndex = cellStartPos; idIndex < maxPos; idIndex++) {
						if (particleIds[idIndex] == idx)
							continue;

						Particle& particle2 = particles[particleIds[idIndex]];
						glm::dvec3 p1p2 = particle.pos - particle2.pos;
						double distance2 = glm::dot(p1p2, p1p2);
						if (distance2 > particleD2 || distance2 < 1e-8)
							continue;

						double distance = sqrt(distance2);
						double tmp = (particleD - distance) / distance;
						glm::dvec3 offset = p1p2 * tmp * 0.5;
						particle.pos += offset;
						particle2.pos -= offset;
						particle.pos.x = std::clamp(particle.pos.x, particleLow.x, particleHigh.x);
						particle.pos.y = std::clamp(particle.pos.y, particleLow.y, particleHigh.y);
						particle.pos.z = zConst ? zConstVal : std::clamp(particle.pos.z, particleLow.z, particleHigh.z);
						particle2.pos.x = std::clamp(particle2.pos.x, particleLow.x, particleHigh.x);
						particle2.pos.y = std::clamp(particle2.pos.y, particleLow.y, particleHigh.y);
						particle2.pos.z = zConst ? zConstVal : std::clamp(particle2.pos.z, particleLow.z, particleHigh.z);
					}
				}
			}
		}
	});
}

void HashedParticles::forEachAround(const glm::ivec3& cellGridCoord, int axis, std::function<void(Particle&)>&& lambda) {
	int cellIndex = cellGridCoord.x * cellNumFace.y * cellNumFace.z + cellGridCoord.y * cellNumFace.z + cellGridCoord.z;
	int max = particleFaceCells[axis][cellIndex + 1];
	for (int p = particleFaceCells[axis][cellIndex]; p < max; p++) {
		lambda(particles[particleFaceIds[axis][p]]);
	}
}

void HashedParticles::forEach(bool parallel, std::function<void(Particle&, int)>&& lambda) {
	int particleNum = particles.size();
	if (parallel) {
#pragma omp parallel for
		for (int p = 0; p < particleNum; p++) {
			lambda(particles[p], p);
		}
	}
	else {
		for (int p = 0; p < particleNum; p++) {
			lambda(particles[p], p);
		}
	}
}

void HashedParticles::setParticleNum(int num) {
	particleIds = std::vector<int>(num);
	while (num < particles.size()) {
		particles.pop_back();
	}
	while (num > particles.size()) {
		Particle particle;
		particle.pos = glm::dvec3(
			util::getDoubleInRange(1.1 * r + cellD.x, dimensions.x - 1.1 * r - cellD.x),
			util::getDoubleInRange(1.1 * r + cellD.y, dimensions.y - 1.1 * r - cellD.y),
			zConst ? z : util::getDoubleInRange(1.1 * r + cellD.z, dimensions.z - 1.1 * r - cellD.z));
		particle.c[0] = particle.c[1] = particle.c[2] = glm::dvec3(0, 0, 0);
		particle.v = glm::dvec3(0, 0, 0);
		particles.push_back(std::move(particle));
	}
	initParticleFaceHash();
	initParticleIntersectionHash();
	updateParticleIntersectionHash(false);
	updateParticleFaceHash(true);
}

void HashedParticles::addParticles(std::vector<Particle>&& particles) {
	this->particles.insert(this->particles.end(), particles.begin(), particles.end());
	initParticleIntersectionHash();
}

void HashedParticles::removeParticles(std::vector<int>&& particleIds) {
	std::sort(particleIds.begin(), particleIds.end());
	const int particleNum = particles.size();
	const int particleIdNum = particleIds.size();
	int particleIdIndex = 0;
	for (int p = 0; p < particleNum; p++) {
		if (particleIdIndex < particleIdNum && p == particleIds[particleIdIndex]) {
			particleIdIndex++;
			continue;
		}
		this->particles[p - particleIdIndex] = std::move(this->particles[p]);
	}
	particles.erase(particles.end() - particleIdNum, particles.end());
	initParticleIntersectionHash();
}

double HashedParticles::getParticleR() const {
	return r;
}

Particle& HashedParticles::getParticleAt(int idx) {
	return particles[idx];
}

int HashedParticles::getParticleNum() const {
	return particles.size();
}

void HashedParticles::setParticleR(double r) {
	this->r = r;
	rInv = 1 / r;
	d = r * 2;
	dInv = 1 / d;
	initParticleIntersectionHash();
	updateParticleIntersectionHash(false);
}

void HashedParticles::updateParticleIntersectionHash(bool parallel) {
	int particleCellNum = particleCells.size();
	int particleNum = particles.size();
	if (parallel) {
#pragma omp parallel for
		for (int i = 0; i < particleCellNum; i++) {
			particleCells[i] = 0;
		}
	}
	else {
		for (int i = 0; i < particleCellNum; i++) {
			particleCells[i] = 0;
		}
	}
	if (parallel) {
#pragma omp parallel for
		for (int p = 0; p < particleNum; p++) {
			glm::ivec3 coord = getCellCoord(particles[p].pos, dInv);
			particleCells[coord.x * cellNum.y * cellNum.z + coord.y * cellNum.z + coord.z].value++;
		}
	}
	else {
		for (int p = 0; p < particleNum; p++) {
			glm::ivec3 coord = getCellCoord(particles[p].pos, dInv);
			particleCells[coord.x * cellNum.y * cellNum.z + coord.y * cellNum.z + coord.z].value++;
		}
	}
	int lastIndex = 0;
	for (int i = 0; i < particleCellNum; i++) {
		lastIndex += particleCells[i];
		particleCells[i] = lastIndex;
	}
	if (parallel) {
#pragma omp parallel for
		for (int p = 0; p < particleNum; p++) {
			Particle& particle = particles[p];
			glm::ivec3 coord = getCellCoord(particle.pos, dInv);
			int index = --particleCells[coord.x * cellNum.y * cellNum.z + coord.y * cellNum.z + coord.z].value;
			particleIds[index] = p;
		}
	}
	else {
		for (int p = 0; p < particleNum; p++) {
			Particle& particle = particles[p];
			glm::ivec3 coord = getCellCoord(particle.pos, dInv);
			int index = --particleCells[coord.x * cellNum.y * cellNum.z + coord.y * cellNum.z + coord.z].value;
			particleIds[index] = p;
		}
	}
}

void HashedParticles::updateParticleFaceHash(bool parallel) {
	/*if (parallel) {
#pragma omp parallel for
		for (int axis = 0; axis < 3; axis++) {
			faceAndCenterHashUpdate(axis);
		}
	}
	else {
		for (int axis = 0; axis < 3; axis++) {
			faceAndCenterHashUpdate(axis);
		}
	}*/
}

void HashedParticles::updateParticleCenterHash() {
	faceAndCenterHashUpdate(3);
}

void HashedParticles::initParticleFaceHash() {
	/*cellNumFace.x = std::ceil(dimensions.x / cellD.x) + 1;
	cellNumFace.y = std::ceil(dimensions.y / cellD.y) + 1;
	cellNumFace.z = std::ceil(dimensions.z / cellD.z) + 1;
	int cellNumTotal = cellNumFace.x * cellNumFace.y * cellNumFace.z + 1;
	for (int axis = 0; axis < 4; axis++) {
		while (particles.size() * 8 < particleFaceIds[axis].size()) {
			particleFaceIds[axis].pop_back();
		}
		while (particles.size() * 8 > particleFaceIds[axis].size()) {
			particleFaceIds[axis].push_back(0);
		}
		while (particleFaceCells[axis].size() < cellNumTotal) {
			particleFaceCells[axis].push_back(0);
		}
		while (particleFaceCells[axis].size() > cellNumTotal) {
			particleFaceCells[axis].pop_back();
		}
	}

	faceAndCenterHashUpdate = [this](int axis) {
		auto& particleCells = particleFaceCells[axis];
		auto& particleIds = particleFaceIds[axis];
		for (int& i : particleCells) {
			i = 0;
		}
		for (auto& particle : particles) {
			glm::ivec3 coord = getCellMinCoord(particle.pos, cellDInv, axis);
			particleCells[coord.x * cellNumFace.y * cellNumFace.z + coord.y * cellNumFace.z + coord.z]++;
			particleCells[coord.x * cellNumFace.y * cellNumFace.z + coord.y * cellNumFace.z + coord.z + 1]++;
			particleCells[coord.x * cellNumFace.y * cellNumFace.z + (coord.y + 1) * cellNumFace.z + coord.z]++;
			particleCells[coord.x * cellNumFace.y * cellNumFace.z + (coord.y + 1) * cellNumFace.z + coord.z + 1]++;
			particleCells[(coord.x + 1) * cellNumFace.y * cellNumFace.z + coord.y * cellNumFace.z + coord.z]++;
			particleCells[(coord.x + 1) * cellNumFace.y * cellNumFace.z + coord.y * cellNumFace.z + coord.z + 1]++;
			particleCells[(coord.x + 1) * cellNumFace.y * cellNumFace.z + (coord.y + 1) * cellNumFace.z + coord.z]++;
			particleCells[(coord.x + 1) * cellNumFace.y * cellNumFace.z + (coord.y + 1) * cellNumFace.z + coord.z + 1]++;
		}
		int lastIndex = 0;
		for (int& i : particleCells) {
			lastIndex += i;
			i = lastIndex;
		}
		for (int p = 0; p < particles.size(); p++) {
			Particle& particle = particles[p];
			glm::ivec3 coord = getCellMinCoord(particle.pos, cellDInv, axis);
			particleIds[--particleCells[coord.x * cellNumFace.y * cellNumFace.z + coord.y * cellNumFace.z + coord.z]] = p;
			particleIds[--particleCells[coord.x * cellNumFace.y * cellNumFace.z + coord.y * cellNumFace.z + coord.z + 1]] = p;
			particleIds[--particleCells[coord.x * cellNumFace.y * cellNumFace.z + (coord.y + 1) * cellNumFace.z + coord.z]] = p;
			particleIds[--particleCells[coord.x * cellNumFace.y * cellNumFace.z + (coord.y + 1) * cellNumFace.z + coord.z + 1]] = p;
			particleIds[--particleCells[(coord.x + 1) * cellNumFace.y * cellNumFace.z + coord.y * cellNumFace.z + coord.z]] = p;
			particleIds[--particleCells[(coord.x + 1) * cellNumFace.y * cellNumFace.z + coord.y * cellNumFace.z + coord.z + 1]] = p;
			particleIds[--particleCells[(coord.x + 1) * cellNumFace.y * cellNumFace.z + (coord.y + 1) * cellNumFace.z + coord.z]] = p;
			particleIds[--particleCells[(coord.x + 1) * cellNumFace.y * cellNumFace.z + (coord.y + 1) * cellNumFace.z + coord.z + 1]] = p;
		}
	};*/
}

void HashedParticles::initParticleIntersectionHash() {
	while (particles.size() < particleIds.size()) {
		particleIds.pop_back();
	}
	while (particles.size() > particleIds.size()) {
		particleIds.push_back(0);
	}
	cellNum.x = std::ceil(dimensions.x * dInv);
	cellNum.y = std::ceil(dimensions.y * dInv);
	cellNum.z = std::ceil(dimensions.z * dInv);
	int cellNumTotal = cellNum.x * cellNum.y * cellNum.z + 1;
	while (particleCells.size() < cellNumTotal) {
		particleCells.push_back(0);
	}
	while (particleCells.size() > cellNumTotal) {
		particleCells.pop_back();
	}
}

void HashedParticles::updateGridParams(const glm::dvec3& cellD, const glm::dvec3& dimensions) {
	this->cellD = cellD;
	this->dimensions = dimensions;
	cellDInv = 1.0 / cellD;
	for (Particle& particle : particles) {
		particle.pos = glm::dvec3(
			std::clamp(particle.pos.x, 1.1 * r + cellD.x, dimensions.x - 1.1 * r - cellD.x),
			std::clamp(particle.pos.y, 1.1 * r + cellD.y, dimensions.y - 1.1 * r - cellD.y),
			zConst ? z : std::clamp(particle.pos.z, 1.1 * r + cellD.z, dimensions.z - 1.1 * r - cellD.z));
	}
	updateParticleIntersectionHash(false);
	initParticleFaceHash();
	updateParticleFaceHash(true);
}
