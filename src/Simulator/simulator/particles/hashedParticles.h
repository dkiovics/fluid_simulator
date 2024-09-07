#pragma once

#include <glm/glm.hpp>
#include "particle.h"
#include <functional>
#include <vector>
#include <array>
#include <atomic>


namespace genericfsim::particles {

/**
 * A class that stores a well defined, bounded volume of particle.
 * The particles can be accessed by their position using the hash functionality of the class.
 */
class HashedParticles {
public:

	/**
	 * Constructs the HashedParticle class.
	 * 
	 * \param num - The number of particles initially in the volume
	 * \param r	- The initial particle radius
	 * \param dimensions - The size of the bounding box (must be same for the MacGrid)
	 * \param cellD - the size of a cell
	 * \param zConst - if true than the z coordinate of all particles is the same number (useful for 2D simulations)
	 * \param z - the z coordinate of all particles if zConst is true
	 */
	HashedParticles(int num, double r, glm::dvec3 dimensions, glm::dvec3 cellD, bool zConst, double z);

	/**
	 * Calls the lambda function for each particle that has a chance for touching the given particle
	 * 
	 * \param particle - the particle around which the particles are checked
	 * \param pIndex - the index of the particle (for optimization purposes)
	 * \param lambda - the called function
	 */
	void forEachIntersecting(const Particle& particle, int pIndex, std::function<void(Particle&)>&& lambda);

	/**
	 * Pushes particles apart, before calling it particle intersection hash must be updates.
	 * 
	 * \param parallel - whearher to run the loop in parallel
	 */
	void pushParticlesApart(bool parallel);

	/**
	 * Calls the lambda function for each particle that is closer to the given face center than cellD.
	 * 
	 * \param cellIndex - the index of the face's corresponding cell
	 * \param axis - the axis of the cell (this defines the face), 3 represents the center of the cell
	 * \param lambda - the called function
	 */
	void forEachAround(const glm::ivec3& cellIndex, int axis, std::function<void(Particle&)>&& lambda);

	/**
	 * Calls a lambda function for each particle.
	 * 
	 * \param parallel - if true the loop runs in parallel
	 * \param lambda - the called function
	 */
	void forEach(bool parallel, std::function<void(Particle&, int)>&& lambda);

	/**
	 * Sets the number of particles, adds or removes particles randomly.
	 * 
	 * \param num - the target number of particles
	 */
	void setParticleNum(int num);

	/**
	 * Returns the particle with a certain index.
	 * 
	 * \param idx - the index of the particle
	 * \return - a reference to the particle
	 */
	Particle& getParticleAt(int idx);

	/**
	 * Returns the number of the particles.
	 * 
	 * \return - the number of the particles
	 */
	int getParticleNum() const;

	/**
	 * Returns the radius of the particles.
	 * 
	 * \return - the radius of the particles
	 */
	double getParticleR() const;

	/**
	 * Sets the radius of the particles, updates the particle hash.
	 * 
	 * \param r - the new particle radius
	 */
	void setParticleR(double r);

	/**
	 * Updates the hash function for the particle intersection according to their new position.
	 */
	void updateParticleIntersectionHash(bool parallel);

	/**
	 * Updates the particle - grid face hash function.
	 */
	void updateParticleFaceHash(bool parallel);

	/**
	 * Updates the particle - grid cell center hash function.
	 * 
	 */
	void updateParticleCenterHash();

	/**
	 * Updates the cellD and grid dimensions, makes sure that no particle is outside the solid wall boundaries.
	 * 
	 * \param cellD - the new cellD
	 * \param dimensions - the new grid dimensions
	 */
	void updateGridParams(const glm::dvec3& cellD, const glm::dvec3& dimensions);

	/**
	 * Ads the particles to the particle collection.
	 * 
	 * \param particles - the particles to be added
	 */
	void addParticles(std::vector<Particle>&& particles);

	/**
	 * Removes the paricles defined by the indexes from the collection.
	 * 
	 * \param particleIds - the indexes of the particles to be removed
	 */
	void removeParticles(std::vector<int>&& particleIds);

private:
	void initParticleIntersectionHash();
	void initParticleFaceHash();

	struct AtomicIntWrapper {
		std::atomic<int> value;

		AtomicIntWrapper(int value) : value(value) { }
		AtomicIntWrapper(AtomicIntWrapper&& other) noexcept : value(other.value.load()) { }
		AtomicIntWrapper(const AtomicIntWrapper& other) : value(other.value.load()) { }
		operator int() const { return value.load(); }
		int operator=(int value) { this->value.store(value); return value; }
	};

	std::vector<Particle> particles;
	std::vector<AtomicIntWrapper> particleCells;
	std::vector<int> particleIds;

	glm::dvec3 dimensions;

	std::array<std::vector<int>, 4> particleFaceCells;
	std::array<std::vector<int>, 4> particleFaceIds;

	glm::ivec3 cellNum = glm::ivec3(0, 0, 0);
	glm::ivec3 cellNumFace = glm::ivec3(0, 0, 0);

	glm::dvec3 cellD;
	glm::dvec3 cellDInv;

	std::function<void(int)> faceAndCenterHashUpdate;

	double r;
	double rInv;
	double d;
	double dInv;

public:
	const double z;
	const bool zConst;

};


}

