#pragma once

#include "compute/storageBuffer.h"
#include "controlRegistry.h"
#include "simulator/macGrid/obstacles.hpp"

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <chrono>

namespace genericfsim::particles
{
class HashedParticles;
}

namespace genericfsim::macgrid
{
class MacGrid;
}

namespace genericfsim::simulator
{
class Simulator;
}

namespace genericfsim::manager
{

struct ParticleSSBOData
{
	glm::vec4 posAndDensity;
	glm::vec4 velocity;
};

struct SimulatorCopy
{
	std::shared_ptr<genericfsim::simulator::Simulator> simulator;
	std::shared_ptr<genericfsim::macgrid::MacGrid> macGrid;
	std::shared_ptr<genericfsim::particles::HashedParticles> hashedParticles;
};

/**
 * A class that represents a simulation (in 2D mode or in full 3D), it manages all the objects necessary for the simulation.
 */
class SimulationManager
{
public:
	/**
	 * Constructs a new SimulationManager.
	 *
	 * \param dimensions - the size of the grid in space
	 * \param twoD - if true, the simulation runs in 2D mode (the z axis is fixed size, fixed pos)
	 */
	SimulationManager(const glm::dvec3& dimensions, bool twoD);

	void stepSimulation(double dt);

	/**
	 * Returns the size of the grid.
	 *
	 * \return - the size of the grid (cellnum for each axis)
	 */
	glm::ivec3 getGridSize() const noexcept;

	/**
	 * Gets a reference for a paricle with a certain index.
	 *
	 * \param index - the particle index
	 * \return - a const ref to the particle
	 */
	//const genericfsim::particles::Particle& getParticleData(int index) const;

	/**
	 * Gets the index of the particle based on the pos.
	 *
	 * \param pos - the point in space, which is inside the particle
	 * \return - the particle index
	 */
	//int getParticleIndex(const glm::dvec3& pos) const;

	/**
	 * Gets a const cell ref base on position in space.
	 *
	 * \param pos - position in space
	 * \return - const ref for the cell
	 */
	//const genericfsim::macgrid::MacGridCell& getCellAt(const glm::dvec3& pos) const;

	/**
	 * Gets a const cell ref base on the grid cell indexes.
	 *
	 * \return - const ref for the cell
	 */
	//const genericfsim::macgrid::MacGridCell& getCellAt(int x, int y, int z) const;

	/**
	 * Returns the dimensions in space of the simulation (determined by the gridResolution, as close as possible to the constructor dimensions).
	 *
	 * \return - the grid dimensions in space
	 */
	glm::dvec3 getDimensions() const noexcept;

	/**
	 * Returns the particle count.
	 *
	 * \return - the particle count
	 */
	int getParticleNum() const noexcept;

	/**
	 * Returns the dimensions of a single cell in space.
	 *
	 * \return - the cell dimensions
	 */
	glm::dvec3 getCellD() const noexcept;

	/**
	 * Returns the particle data in the form of a SSBO.
	 * 
	 * \return the particle data in the form of a SSBO
	 */
	renderer::ssbo_ptr<ParticleSSBOData> getParticleData() const noexcept;

	/**
	 * Sets the particle data in the form of a SSBO.
	 * 
	 * \param particleData - the particle data in the form of a SSBO
	 */
	void setParticleData(renderer::ssbo_ptr<ParticleSSBOData> particleData);

	/**
	 * Computes the particle density for each particle and stores it in the SSBO.
	 * 
	 * \param particleData - the particle data in the form of a SSBO from which the density is computed
	 */
	void computeParticleDensities(renderer::ssbo_ptr<ParticleSSBOData> particleData);

	const bool twoD;

	void setObstacles(std::vector<std::unique_ptr<genericfsim::obstacle::Obstacle>> obstacles);

private:
	std::shared_ptr<genericfsim::simulator::Simulator> simulator;
	std::shared_ptr<genericfsim::macgrid::MacGrid> macGrid;
	std::shared_ptr<genericfsim::particles::HashedParticles> hashedParticles;
	std::shared_ptr<genericfsim::particles::HashedParticles> hashedParticlesCopy;
	const glm::dvec3 dimensions;

	bool autoDt = true;
	double dtVal = 0.01;
	bool run = false;

	renderer::ssbo_ptr<ParticleSSBOData> particleData;

	int particleNum;

	controls::ControlRegistry& controlRegistry;

	std::vector<std::unique_ptr<genericfsim::obstacle::Obstacle>> obstacles;

	void updateMacGridParams();
};


}
