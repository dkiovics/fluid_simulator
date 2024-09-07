#pragma once

#include <glm/glm.hpp>
#include "macGrid/macGrid.h"
#include "particles/hashedParticles.h"
#include <memory>
#include <map>
#include <string>


namespace genericfsim::simulator {

/**
 * The fluid simulation class.
 */
class Simulator {
public:

	/**
	 * Enum for the Particle-Grid-Particle transfer type.
	 */
	enum class P2G2PType {
		PIC, FLIP, APIC
	};


	/**
	 * A struct to easily store, update and set the Simulator's config.
	 */
	struct SimulatorConfig {
		P2G2PType transferType = P2G2PType::FLIP;
		float flipRatio = 0.99;
		float gravity = 150.0;
		bool gravityEnabled = true, pushParticlesApartEnabled = true;
		bool pushApartEnabled = true;
		bool particleSpawningEnabled = false;
		bool particleDespawningEnabled = false;
		bool stopParticles = false;
	};

	/**
	 * Constructs the Simulator class.
	 * 
	 * \param config - the initial config of the simulator
	 * \param hashedParticles - pointer to a HashedParticles object
	 * \param macGrid - pointer to an IMacGrid object
	 */
	Simulator(SimulatorConfig config, std::shared_ptr<genericfsim::particles::HashedParticles> hashedParticles, std::shared_ptr<genericfsim::macgrid::MacGrid> macGrid);

	/**
	 * Sets a new MacGrid instance for simulation.
	 * 
	 * \param macGrid - the new MacGrid instance
	 */
	void setNewMacGrid(std::shared_ptr<genericfsim::macgrid::MacGrid> macGrid);

	/**
	 * Sets a new HashedParticles instance for simulation.
	 * 
	 * \param particles - the new HashedParticles instance (this basically restarts the simulation)
	 */
	void setNewHashedParticles(std::shared_ptr<genericfsim::particles::HashedParticles> particles);

	/**
	 * Executes a simulation iteration that is dt time long.
	 * 
	 * \param dt - the time step size in s
	 */
	void simulate(double dt);

	/**
	 * Stores all obstacles. Obstacle speed, prevPos and pos need to be updated externally.
	 */
	std::vector<std::unique_ptr<genericfsim::obstacle::Obstacle>> obstacles;

	/**
	 * Returns the duration of each simulation step in the last iteration.
	 * 
	 * \return - a map with step name - duration pairs
	 */
	std::map<std::string, long long> getStepDuration() const;

public:
	SimulatorConfig config;

private:
	std::shared_ptr<genericfsim::particles::HashedParticles> hashedParticles;
	std::shared_ptr<genericfsim::macgrid::MacGrid> macGrid;

	std::map <std::string, long long> stepDuration;

	void spawnParticles(double dt);
	void advectParticles(bool parallel, double dt);
	void pushParticlesOutOfObstacles(bool parallel);
	void p2gTransfer(bool parallel, double dt);
	void markFluidCellsAndCalculateParticleDensities(bool parallel);
	void addObstaclesToGrid(bool parallel);
	void g2pTransfer(bool parallel);
};


}
