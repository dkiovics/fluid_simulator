#pragma once

#include "../simulator/simulator.h"
#include "../simulator/macGrid/basicMacGrid.h"
#include "../simulator/macGrid/bridsonSolverGrid.h"
#include "../simulator/particles/hashedParticles.h"

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <condition_variable>

namespace genericfsim::manager {

using SimulatorConfig = genericfsim::simulator::Simulator::SimulatorConfig;
using P2G2PType = genericfsim::simulator::Simulator::P2G2PType;
using RectengularObstacle = genericfsim::obstacle::RectengularObstacle;
using SphericalObstacle = genericfsim::obstacle::SphericalObstacle;
using Obstacle = genericfsim::obstacle::Obstacle;

struct SimulationConfig {
	float gridResolution;
	float particleRadius;
	bool isTopOfContainerSolid;
	float pressureK, averagePressure;
	int incompressibilityIterationCount;
	SimulatorConfig simulatorConfig;
	bool pressureEnabled;
	float residualTolerance = 1e-6;
	float fluidDensity = 1.0;
	
	enum class GridSolverType {
		BRIDSON, BASIC
	};
	GridSolverType gridSolverType = GridSolverType::BRIDSON;
};

/**
 * A class that represents a simulation (in 2D mode or in full 3D), it manages all the objects necessary for the simulation.
 * The simulation is being run by a threadworker, so the class also manages all the communication between the external and internal thread.
 */
class SimulationManager {
public:
	/**
	 * Constructs a new SimulationManager.
	 * 
	 * \param dimensions - the size of the grid in space
	 * \param config - the initial config
	 * \param particleCount - the initial particle count
	 * \param twoD - if true, the simulation runs in 2D mode (the z axis is fixed size, fixed pos)
	 */
	SimulationManager(const glm::dvec3& dimensions, const SimulationConfig& config, int particleCount, bool twoD);

	/**
	 * Starts the simulation on another thread.
	 */
	void startSimulation();

	/**
	 * Restarts the simulation (resets the particle positions).
	 */
	void restartSimulation();

	/**
	 * Returns the size of the grid.
	 * 
	 * \return - the size of the grid (cellnum for each axis)
	 */
	glm::ivec3 getGridSize() const;

	struct ParticleGfxData {
		glm::vec3 pos;
		float v;
		float density;
	};
	/**
	 * Returns the gfx data of all particles.
	 * 
	 * \return - an array with all the particle positions and speeds (speeds are used for visualization).
	 */
	std::vector<ParticleGfxData> getParticleGfxData();

	/**
	 * Sets whether or not to update the particle speeds in the ParticleGfxData array in each iteration.
	 * 
	 * \param calculate - if true, the speeds will be calculated
	 */
	void setCalculateParticleSpeeds(bool calculate);

	/**
	 * \brief Sets whether the particle densities should be calculated.
	 * 
	 * \param calculate - if true, the densities will be calculated
	 */
	void setCalculateParticleDensities(bool calculate);

	/**
	 * Gets a reference for a paricle with a certain index. Be careful, because the particle data might be changed by another thread.
	 * 
	 * \param index - the particle index
	 * \return - a const ref to the particle
	 */
	const genericfsim::particles::Particle& getParticleData(int index);

	/**
	 * Gets the index of the particle based on the pos.
	 * 
	 * \param pos - the point in space, which is inside the particle
	 * \return - the particle index
	 */
	int getParticleIndex(const glm::dvec3& pos);

	/**
	 * Gets a const cell ref base on position in space. Be careful, because the particle data might be changed by another thread.
	 * 
	 * \param pos - position in space
	 * \return - const ref for the cell
	 */
	const genericfsim::macgrid::MacGridCell& getCellAt(const glm::dvec3& pos);

	/**
	 * Gets a const cell ref base on the grid cell indexes. Be careful, because the particle data might be changed by another thread.
	 *
	 * \return - const ref for the cell
	 */
	const genericfsim::macgrid::MacGridCell& getCellAt(int x, int y, int z);

	/**
	 * Returns all the Obstacles currently stored in the simulator.
	 * 
	 * \return - an array of the obstacles (these are just clones)
	 */
	std::vector<std::unique_ptr<Obstacle>> getObstacles();

	/**
	 * Sets a new array for the obstacles in the simulator.
	 * 
	 * \param obstacles - the obstacle array, must be moved, because of the unique_ptr
	 */
	void setObstacles(std::vector<std::unique_ptr<Obstacle>>&& obstacles);

	/**
	 * Returns the simulator config.
	 * 
	 * \return - the config that is currently set (might not be active yet)
	 */
	SimulationConfig getConfig();

	/**
	 * Sets the simulator config.
	 * 
	 * \param config - the new config, it will be the active one in the next simulation iteration
	 */
	void setConfig(const SimulationConfig& config);

	/**
	 * Sets whether the simulation dt is determined by the iteration length.
	 * 
	 * \param autoDt
	 */
	void setAutoDt(bool autoDt);
	
	/**
	 * Sets whether the simulation runs (otherwise it can be stepped).
	 * 
	 * \param run - if true then the simulation is running
	 */
	void setRun(bool run);

	/**
	 * Steps the simulation if it is currently not running.
	 */
	void stepSimulation();

	/**
	 * Sets the dt in case the autoDt is off.
	 * 
	 * \param dt - the dt for each iteration
	 */
	void setSimulationDt(double dt);

	/**
	 * Returns the duration of each simulation step.
	 * 
	 * \return - the duration of each major simulation step
	 */
	std::map<std::string, long long> getStepDuration();

	/**
	 * Returns the dimensions in space of the simulation (determined by the gridResolution, as close as possible to the constructor dimensions).
	 * 
	 * \return - the grid dimensions in space
	 */
	glm::dvec3 getDimensions() const;

	/**
	 * Returns the last simulation iteration length.
	 * 
	 * \return - the length in seconds
	 */
	double getLastFrameTime() const;

	/**
	 * Returns the particle count currently targeted.
	 * 
	 * \return - the particle count
	 */
	int getParticleNum();

	/**
	 * Sets the new particle count.
	 * 
	 * \param count - the new particle count
	 */
	void setParticleNum(int count);

	/**
	 * Returns the dimensions of a single cell in space.
	 * 
	 * \return - the cell dimensions
	 */
	glm::dvec3 getCellD();


	const bool twoD;

	~SimulationManager();

private:
	std::shared_ptr<genericfsim::simulator::Simulator> simulator;
	std::shared_ptr<genericfsim::macgrid::MacGrid> macGrid;
	std::shared_ptr<genericfsim::particles::HashedParticles> hashedParticles;
	const glm::dvec3 dimensions;

	void simulationThreadWorker();

private:
	//Shared variables between the two threads
	std::mutex sharedDataMutex;

	std::condition_variable simulationStepVar;
	
	std::atomic<bool> calculateParticleSpeeds = false;
	std::atomic<bool> calculateParticleDensities = false;
	std::atomic<bool> autoDt = true;
	std::atomic<double> dtVal = 0.01;
	std::atomic<bool> run = false;
	std::atomic<bool> terminationRequest = false;
	bool restart = false;
	std::unique_ptr<std::thread> simulationThread;

	std::vector<ParticleGfxData> particleData;

	std::map<std::string, long long> durations;

	std::vector<std::unique_ptr<Obstacle>> obstacles;

	SimulationConfig config;
	SimulationConfig currentConfig;
	int particleNum;
	int currentParticleNum;

	std::atomic<double> lastIterationDuration = 0.01;
};


}
