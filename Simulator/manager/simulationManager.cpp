#include "simulationManager.h"
#include <chrono>

using namespace genericfsim::manager;
using namespace genericfsim::macgrid;
using namespace genericfsim::particles;
using namespace genericfsim::simulator;

SimulationManager::SimulationManager(const glm::dvec3& dimensions, const SimulationConfig& config, int particleNum, bool twoD)
	: dimensions(dimensions), twoD(twoD), particleNum(particleNum) {
	this->config = config;
	this->currentConfig = config;
	this->currentParticleNum = particleNum;

	if(config.gridSolverType == SimulationConfig::GridSolverType::BRIDSON)
		macGrid = std::make_shared<BridsonSolverGrid>(dimensions, config.gridResolution, twoD, config.fluidDensity);
	else
		macGrid = std::make_shared<BasicMacGrid>(dimensions, config.gridResolution, twoD);
	macGrid->averagePressure = config.averagePressure;
	macGrid->incompressibilityMaxIterationCount = config.incompressibilityIterationCount;
	macGrid->isTopOfContainerSolid = config.isTopOfContainerSolid;
	macGrid->pressureEnabled = config.pressureEnabled;
	macGrid->pressureK = config.pressureK;
	macGrid->residualTolerance = config.residualTolerance;

	hashedParticles = std::make_shared<HashedParticles>(particleNum, config.particleRadius, macGrid->dimensions, macGrid->cellD, twoD, dimensions.z / 2);
	simulator = std::make_shared<Simulator>(config.simulatorConfig, hashedParticles, macGrid);
}

void SimulationManager::setConfig(const SimulationConfig& config) {
	std::unique_lock lock(sharedDataMutex);
	this->config = config;
}

glm::ivec3 SimulationManager::getGridSize() const {
	return macGrid->gridSize;
}

void SimulationManager::setCalculateParticleSpeeds(bool calculate) {
	std::unique_lock lock(sharedDataMutex);
	calculateParticleSpeeds = calculate;
}

std::vector<SimulationManager::ParticleGfxData> SimulationManager::getParticleGfxData() {
	std::unique_lock lock(sharedDataMutex);
	return particleData;
}

void SimulationManager::startSimulation() {
	if (simulationThread)
		return;
	run = true;
	simulationThread = std::make_unique<std::thread>([this]() {
		simulationThreadWorker();
	});
}

void SimulationManager::restartSimulation() {
	restart = true;
}

const genericfsim::particles::Particle& SimulationManager::getParticleData(int index) {
	std::unique_lock lock(sharedDataMutex);
	if (index < hashedParticles->getParticleNum())
		return hashedParticles->getParticleAt(index);
	return hashedParticles->getParticleAt(0);
}

int SimulationManager::getParticleIndex(const glm::dvec3& pos) {
	std::unique_lock lock(sharedDataMutex);
	double r = hashedParticles->getParticleR();
	double r2 = r * r;
	for (int p = 0; p < currentParticleNum; p++) {
		glm::dvec3 p2p = pos - hashedParticles->getParticleAt(p).pos;
		double distance2 = glm::dot(p2p, p2p);
		if (distance2 < r2)
			return p;
	}
	return 0;
}

void SimulationManager::setAutoDt(bool autoDt) {
	this->autoDt = autoDt;
}

void SimulationManager::setRun(bool run) {
	this->run = run;
	if(run)
		simulationStepVar.notify_all();
}

void SimulationManager::stepSimulation() {
	simulationStepVar.notify_all();
}

void SimulationManager::setSimulationDt(double dt) {
	this->dtVal = dt;
}

std::map<std::string, long long> SimulationManager::getStepDuration() {
	std::unique_lock lock(sharedDataMutex);
	return simulator->getStepDuration();
}

glm::dvec3 SimulationManager::getCellD() {
	std::unique_lock lock(sharedDataMutex);
	return macGrid->cellD;
}

const MacGridCell& SimulationManager::getCellAt(const glm::dvec3& pos) {
	std::unique_lock lock(sharedDataMutex);
	const glm::dvec3 gridPos = pos * macGrid->cellDInv;
	if(gridPos.x < getGridSize().x && gridPos.y < getGridSize().y && (gridPos.z < getGridSize().z || macGrid->twoD))
		return macGrid->cell(gridPos.x, gridPos.y, macGrid->twoD ? 1 : gridPos.z);
	return macGrid->cell(0, 0, 1);
}

const MacGridCell& SimulationManager::getCellAt(int x, int y, int z) {
	std::unique_lock lock(sharedDataMutex);
	if (x < getGridSize().x && y < getGridSize().y && (z < getGridSize().z || macGrid->twoD))
		return macGrid->cell(x, y, macGrid->twoD ? 1 : z);
	return macGrid->cell(0, 0, 1);
}

std::vector<std::unique_ptr<Obstacle>> SimulationManager::getObstacles() {
	std::unique_lock lock(sharedDataMutex);
	std::vector<std::unique_ptr<Obstacle>> tmp;
	for (auto& o : obstacles)
		tmp.push_back(std::unique_ptr<Obstacle>(o->clone()));
	return tmp;
}

void SimulationManager::setObstacles(std::vector<std::unique_ptr<Obstacle>>&& obstacles) {
	std::unique_lock lock(sharedDataMutex);
	this->obstacles = std::move(obstacles);
	if (hashedParticles->zConst) {
		for (auto& o : this->obstacles) {
			o->pos.z = o->prevPos.z = hashedParticles->z;
			o->speed.z = 0;
		}
	}
}

int SimulationManager::getParticleNum() {
	std::unique_lock lock(sharedDataMutex);
	return particleNum;
}

void SimulationManager::setParticleNum(int count) {
	std::unique_lock lock(sharedDataMutex);
	particleNum = count;
}

SimulationConfig SimulationManager::getConfig() {
	std::unique_lock lock(sharedDataMutex);
	return config;
}

glm::dvec3 SimulationManager::getDimensions() const {
	return macGrid->dimensions;
}

double SimulationManager::getLastFrameTime() const {
	return lastIterationDuration;
}

SimulationManager::~SimulationManager() {
	terminationRequest = true;
	simulationStepVar.notify_all();
	if (simulationThread)
		simulationThread->join();
}

void SimulationManager::simulationThreadWorker() {
	while (!terminationRequest) {
		double dt = autoDt ? lastIterationDuration : dtVal;
		{
			std::unique_lock lock(sharedDataMutex);

			if (config.gridResolution != currentConfig.gridResolution || config.gridSolverType != currentConfig.gridSolverType) {
				if (config.gridSolverType == SimulationConfig::GridSolverType::BRIDSON)
					macGrid = std::make_shared<BridsonSolverGrid>(dimensions, config.gridResolution, twoD, config.fluidDensity);
				else
					macGrid = std::make_shared<BasicMacGrid>(dimensions, config.gridResolution, twoD);
				hashedParticles->updateGridParams(macGrid->cellD, macGrid->dimensions);
				simulator->setNewMacGrid(macGrid);
			}
			macGrid->averagePressure = config.averagePressure;
			macGrid->incompressibilityMaxIterationCount = config.incompressibilityIterationCount;
			macGrid->isTopOfContainerSolid = config.isTopOfContainerSolid;
			macGrid->pressureEnabled = config.pressureEnabled;
			macGrid->pressureK = config.pressureK;
			macGrid->residualTolerance = config.residualTolerance;
			macGrid->fluidDensity = config.fluidDensity;

			if (particleNum != currentParticleNum) {
				hashedParticles->setParticleNum(particleNum);
				currentParticleNum = particleNum;
			}
			if (currentConfig.particleRadius != config.particleRadius) {
				hashedParticles->setParticleR(config.particleRadius);
			}
			simulator->config = config.simulatorConfig;
			currentConfig = config;

			simulator->obstacles.clear();
			for (auto& o : obstacles) {
				o->calculateSpeed(dt);
				simulator->obstacles.push_back(std::unique_ptr<Obstacle>(o->clone()));
				o->prevPos = o->pos;
			}

			durations = simulator->getStepDuration();

			if (restart) {
				restart = false;
				hashedParticles = std::make_shared<HashedParticles>(particleNum, config.particleRadius,
																	macGrid->dimensions, macGrid->cellD, twoD, dimensions.z / 2);
				simulator->setNewHashedParticles(hashedParticles);
			}

			while (particleData.size() < hashedParticles->getParticleNum())
				particleData.push_back(ParticleGfxData());
			while (particleData.size() > hashedParticles->getParticleNum())
				particleData.pop_back();
			hashedParticles->forEach(true, [&](Particle& p, int index) {
				particleData[index].pos = glm::vec3(p.pos.x, p.pos.y, p.pos.z);
				if (calculateParticleSpeeds)
					particleData[index].v = glm::length(p.v);
			});

			particleNum = currentParticleNum = hashedParticles->getParticleNum();

			if (!run)
				simulationStepVar.wait(lock);
		}
		if (terminationRequest)
			break;

		auto start = std::chrono::high_resolution_clock::now();
		simulator->simulate(dt);
		lastIterationDuration = lastIterationDuration * 0.8 + 0.2 * std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() / 1e6;
	}
}
