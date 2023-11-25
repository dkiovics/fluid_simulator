#pragma once

#include <glm/glm.hpp>
#include <memory>
#include "manager/simulationManager.h"


class GfxInterface {
public:
	bool gridlinesEnabled = false;
	glm::vec3 gridLineColor = glm::vec3(0.4, 0.0, 0.0);
	glm::vec3 particleColor = glm::vec3(0.0, 0.4, 0.95);
	glm::ivec2 screenStart = glm::ivec2(0, 0);
	glm::ivec2 screenSize = glm::ivec2(1000, 1000);

	glm::vec3 particleSpeedColor = glm::vec3(0.4, 0.93, 0.88);
	float maxParticleSpeed = 36.0f;
	bool particleSpeedColorEnabled = true;

	virtual void setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> manager) = 0;

	virtual void handleTimePassed(double dt) { }

	virtual void render() = 0;

	virtual void addSphericalObstacle(glm::vec3 color, float r) { }
	virtual void addRectengularObstacle(glm::vec3 color, glm::vec3 size) { }
	virtual void removeObstacle() { }

	virtual ~GfxInterface() = default;
};
