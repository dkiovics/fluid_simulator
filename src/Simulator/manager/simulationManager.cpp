#include "../simulator/simulator.h"
#include "../simulator/macGrid/basicMacGrid.h"
#include "../simulator/macGrid/bridsonSolverGrid.h"
#include "../simulator/particles/hashedParticles.h"
#include "simulationManager.h"
#include <chrono>
#include "../simulator/util/interpolation.h"

using namespace genericfsim::manager;
using namespace genericfsim::macgrid;
using namespace genericfsim::particles;
using namespace genericfsim::simulator;

static Simulator::SimulatorConfig getConfig(controls::ControlRegistry& controlRegistry)
{
	Simulator::SimulatorConfig config;
	config.transferType = (Simulator::P2G2PType)(int)controlRegistry["sim.transfer_type"];
	config.flipRatio = (float)controlRegistry["sim.flip"];
	config.gravity = (float)controlRegistry["sim.gravity_value"];
	config.gravityEnabled = (bool)controlRegistry["sim.gravity"];
	config.pushApartEnabled = (bool)controlRegistry["sim.push_apart"];
	config.stopParticles = (bool)controlRegistry["sim.stop_particles"];
	return config;
}

SimulationManager::SimulationManager(const glm::dvec3& dimensions, bool twoD)
	: dimensions(dimensions), twoD(twoD), controlRegistry(controls::ControlRegistry::getInstance())
{
	particleNum = (int)controlRegistry["sim.particle_count"];

	updateMacGridParams();

	hashedParticles = std::make_shared<HashedParticles>(particleNum, (float)controlRegistry["sim.particle_radius"], macGrid->dimensions, macGrid->cellD, twoD, dimensions.z / 2);
	simulator = std::make_shared<Simulator>(getConfig(controlRegistry), hashedParticles, macGrid);

	particleData = renderer::make_ssbo<ParticleSSBOData>(particleNum, GL_DYNAMIC_DRAW);

	hashedParticlesCopy = std::make_shared<HashedParticles>(*hashedParticles);
}

void SimulationManager::updateMacGridParams()
{
	bool isBridsonSolver = dynamic_cast<BridsonSolverGrid*>(macGrid.get()) != nullptr;
	float setResolution = (float)controlRegistry["sim.grid_resolution"];
	bool isBridsonSolverSet = (int)controlRegistry["sim.grid_solver"] == 0;
	if (!macGrid || setResolution != macGrid->resolution || isBridsonSolver != isBridsonSolverSet)
	{
		if (isBridsonSolverSet)
			macGrid = std::make_shared<BridsonSolverGrid>(dimensions, setResolution, twoD, (float)controlRegistry["sim.solver_density"]);
		else
			macGrid = std::make_shared<BasicMacGrid>(dimensions, setResolution, twoD);

		if (hashedParticles)
			hashedParticles->updateGridParams(macGrid->cellD, macGrid->dimensions);
		if (hashedParticlesCopy)
			hashedParticlesCopy->updateGridParams(macGrid->cellD, macGrid->dimensions);
		if (simulator)
			simulator->setNewMacGrid(macGrid);
	}
	macGrid->averagePressure = (float)controlRegistry["sim.average_pressure"];
	macGrid->incompressibilityMaxIterationCount = controlRegistry["sim.incompressibility_iterations"];
	macGrid->isTopOfContainerSolid = controlRegistry["sim.top_is_solid"];
	macGrid->pressureEnabled = controlRegistry["sim.pressure"];
	macGrid->pressureK = (float)controlRegistry["sim.pressure_k"];
	macGrid->residualTolerance = (float)controlRegistry["sim.solver_tolerance"];
	macGrid->fluidDensity = (float)controlRegistry["sim.solver_density"];
}

glm::ivec3 SimulationManager::getGridSize() const noexcept
{
	return macGrid->gridSize;
}

renderer::ssbo_ptr<ParticleSSBOData> SimulationManager::getParticleData() const noexcept
{
	return particleData;
}

//const genericfsim::particles::Particle& SimulationManager::getParticleData(int index) const noexcept
//{
//	if (index < hashedParticles->getParticleNum())
//		return hashedParticles->getParticleAt(index);
//	return hashedParticles->getParticleAt(0);
//}
//
//int SimulationManager::getParticleIndex(const glm::dvec3& pos) const noexcept
//{
//	double r = hashedParticles->getParticleR();
//	double r2 = r * r;
//	for (int p = 0; p < particleNum; p++)
//	{
//		glm::dvec3 p2p = pos - hashedParticles->getParticleAt(p).pos;
//		double distance2 = glm::dot(p2p, p2p);
//		if (distance2 < r2)
//			return p;
//	}
//	return 0;
//}

void SimulationManager::setObstacles(std::vector<std::unique_ptr<genericfsim::obstacle::Obstacle>> obstacles)
{
	this->obstacles = std::move(obstacles);
}

void SimulationManager::stepSimulation(double dt)
{
	run = (bool)controlRegistry["sim.run"];
	autoDt = (bool)controlRegistry["sim.auto_dt"];
	dtVal = (float)controlRegistry["sim.dt"];
	particleNum = (int)controlRegistry["sim.particle_count"];

	updateMacGridParams();

	hashedParticles->setParticleNum(particleNum);
	hashedParticles->setParticleR((float)controlRegistry["sim.particle_radius"]);
	hashedParticlesCopy->setParticleNum(particleNum);
	hashedParticlesCopy->setParticleR(hashedParticles->getParticleR());

	simulator->config = getConfig(controlRegistry);

	simulator->obstacles.clear();
	for (auto& o : obstacles)
	{
		o->calculateSpeed(dt);
		simulator->obstacles.push_back(std::unique_ptr<genericfsim::obstacle::Obstacle>(o->clone()));
		o->prevPos = o->pos;
	}

	if ((bool)controlRegistry["sim.restart"])
	{
		controlRegistry["sim.restart"] = false;
		hashedParticles = std::make_shared<HashedParticles>(particleNum, (float)controlRegistry["sim.particle_radius"],
			macGrid->dimensions, macGrid->cellD, twoD, dimensions.z / 2);
		simulator->setNewHashedParticles(hashedParticles);
	}

	particleData->setSize(particleNum);

	particleData->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

	hashedParticles->forEach(true, [&](Particle& p, int index) {
		float density = 0;
		auto cells = macGrid->getCellsAround(p.pos);
		for (auto& c : cells)
			density += float(trilinearInterpoll(p.pos, c.cell.pos, macGrid->cellDInv) * c.cell.avgPNum);
		(*particleData)[index].velocity = glm::vec4(p.v.x, p.v.y, p.v.z, 0);
		(*particleData)[index].posAndDensity = glm::vec4(p.pos.x, p.pos.y, p.pos.z, density);
	});
	particleData->unmapBuffer();

	if (run || controlRegistry["sim.step"])
	{
		controlRegistry["sim.step"] = false;
		simulator->simulate(autoDt ? dt : dtVal);
	}
}

glm::dvec3 SimulationManager::getCellD() const noexcept
{
	return macGrid->cellD;
}

//const MacGridCell& SimulationManager::getCellAt(const glm::dvec3& pos) const noexcept
//{
//	const glm::dvec3 gridPos = pos * macGrid->cellDInv;
//	if (gridPos.x < getGridSize().x && gridPos.y < getGridSize().y && (gridPos.z < getGridSize().z || macGrid->twoD))
//		return macGrid->cell((int)gridPos.x, (int)gridPos.y, (int)macGrid->twoD ? 1 : (int)gridPos.z);
//	return macGrid->cell(0, 0, 1);
//}
//
//const MacGridCell& SimulationManager::getCellAt(int x, int y, int z) const noexcept
//{
//	if (x < getGridSize().x && y < getGridSize().y && (z < getGridSize().z || macGrid->twoD))
//		return macGrid->cell(x, y, macGrid->twoD ? 1 : z);
//	return macGrid->cell(0, 0, 1);
//}

int SimulationManager::getParticleNum() const noexcept
{
	return particleNum;
}

glm::dvec3 SimulationManager::getDimensions() const noexcept
{
	return macGrid->dimensions;
}

void SimulationManager::setParticleData(renderer::ssbo_ptr<ParticleSSBOData> particleData)
{
	if (particleData->getSize() != particleNum)
	{
		throw std::runtime_error("Particle data size does not match the number of particles.");
	}
	particleData->mapBuffer(0, -1, GL_MAP_READ_BIT);
	hashedParticles->forEach(false, [&](Particle& p, int index) {
		const auto& data = (*particleData)[index];
		p.pos = glm::dvec3(data.posAndDensity.x, data.posAndDensity.y, data.posAndDensity.z);
		p.v = glm::dvec3(data.velocity.x, data.velocity.y, data.velocity.z);
	});
	particleData->unmapBuffer();
}

//SimulatorCopy SimulationManager::getSimulatorCopy() const
//{
//	SimulatorCopy copy;
//	copy.hashedParticles = std::make_shared<HashedParticles>(*hashedParticles);
//	copy.macGrid = macGrid->clone();
//	copy.simulator = std::make_shared<Simulator>(config.simulatorConfig, copy.hashedParticles, copy.macGrid);
//	return copy;
//}

void SimulationManager::computeParticleDensities(renderer::ssbo_ptr<ParticleSSBOData> particleData)
{
	if (particleData->getSize() != particleNum)
	{
		throw std::runtime_error("Particle data size does not match the number of particles.");
	}

	particleData->mapBuffer(0, -1, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

	const double r = hashedParticlesCopy->getParticleR();
	const glm::dvec3 cellD = getCellD();
	const glm::dvec3 lowerLimit = cellD + r;
	const glm::dvec3 upperLimit = getDimensions() - cellD - r;
	const glm::dvec3 cellDInv = macGrid->cellDInv;

	macGrid->backupGrid();
	macGrid->resetGridValues(true);

#pragma omp parallel for
	for (int i = 0; i < particleNum; i++)
	{
		const auto& data = (*particleData)[i];
		glm::dvec3 pos = glm::dvec3(data.posAndDensity.x, data.posAndDensity.y, data.posAndDensity.z);
		if (pos.x < lowerLimit.x || pos.y < lowerLimit.y || pos.z < lowerLimit.z ||
			pos.x > upperLimit.x || pos.y > upperLimit.y || pos.z > upperLimit.z)
			continue;
		auto cells = macGrid->getCellsAround(pos);
		for (int p = 0; p < cells.size(); p++)
		{
			double weight = trilinearInterpoll(cells[p].cell.pos, pos, cellDInv);
			cells[p].cell.avgPNum += weight;
		}
	}

#pragma omp parallel for
	for (int i = 0; i < particleNum; i++)
	{
		auto& data = (*particleData)[i];
		glm::dvec3 pos(data.posAndDensity.x, data.posAndDensity.y, data.posAndDensity.z);
		float density = 0;
		if (pos.x >= lowerLimit.x && pos.y >= lowerLimit.y && pos.z >= lowerLimit.z &&
			pos.x <= upperLimit.x && pos.y <= upperLimit.y && pos.z <= upperLimit.z)
		{
			auto cells = macGrid->getCellsAround(pos);
			for (auto& c : cells)
				density += float(trilinearInterpoll(pos, c.cell.pos, cellDInv) * c.cell.avgPNum);
		}
		data.posAndDensity.w = density;
	}

	macGrid->restoreGrid();

	particleData->unmapBuffer();
}