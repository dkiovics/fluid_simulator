#pragma once

#include <glm/glm.hpp>

namespace genericfsim::obstacle {

struct Obstacle {
	glm::dvec3 pos;
	glm::dvec3 prevPos;
	glm::dvec3 speed = glm::dvec3(0, 0, 0);

	Obstacle(const glm::dvec3& pos = glm::dvec3(0, 0, 0)) : pos(pos), prevPos(pos) { }

	void setNewPos(const glm::dvec3& pos) {
		prevPos = this->pos;
		this->pos = pos;
	}

	void calculateSpeed(double dt) {
		speed = (pos - prevPos) / dt;
	}

	virtual Obstacle* clone() = 0;
};

struct RectengularObstacle : public Obstacle {
	const glm::dvec3 size;

	RectengularObstacle(const glm::dvec3& size, const glm::dvec3& pos = glm::dvec3(0, 0, 0)) : Obstacle(pos), size(size) { }

	Obstacle* clone() override {
		return new RectengularObstacle(*this);
	}
};

struct SphericalObstacle : public Obstacle {
	const double r;

	SphericalObstacle(double r, const glm::dvec3& pos = glm::dvec3(0, 0, 0)) : Obstacle(pos), r(r) { }

	Obstacle* clone() override {
		return new SphericalObstacle(*this);
	}
};

struct SphericalParticleSource : public SphericalObstacle {
	const double particleSpawnRate;
	const double particleSpawnSpeed;
	double lastSpawnFraction = 0;

	SphericalParticleSource(double r, double particleSpawnRate, double particleSpawnSpeed, const glm::dvec3& pos = glm::dvec3(0, 0, 0)) 
		: SphericalObstacle(r, pos), particleSpawnRate(particleSpawnRate), particleSpawnSpeed(particleSpawnSpeed) { }

	Obstacle* clone() override {
		return new SphericalParticleSource(*this);
	}
};

struct SphericalParticleSink : public SphericalObstacle {
	SphericalParticleSink(double r, const glm::dvec3& pos = glm::dvec3(0, 0, 0)) : SphericalObstacle(r, pos) { }

	Obstacle* clone() override {
		return new SphericalParticleSink(*this);
	}
};

}
