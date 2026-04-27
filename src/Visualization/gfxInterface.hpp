#pragma once

#include <glm/glm.hpp>
#include "engine/framebuffer.h"
#include "compute/storageBuffer.h"
#include "manager/simulationManager.h"


namespace visual
{

struct SceneConfig
{
	glm::vec3 simSize;
	glm::vec3 simCenter;
	glm::ivec3 gridResolution;

	bool operator==(const SceneConfig& other) const
	{
		return simSize == other.simSize && simCenter == other.simCenter && gridResolution == other.gridResolution;
	}
};

struct Obstacle
{
	enum class ObstacleType
	{
		BOX, SPHERE
	};

	ObstacleType type;
	glm::vec3 size;

	glm::vec3 position;
	glm::vec3 prevPosition;
	glm::vec3 color;

	bool wasMoved = false;

	int meshIndex = -1;

	Obstacle(glm::vec3 size, glm::vec3 position, glm::vec3 color)
		: type(ObstacleType::BOX), size(size), position(position), prevPosition(position), color(color) { }

	Obstacle(float radius, glm::vec3 position, glm::vec3 color)
		: type(ObstacleType::SPHERE), size(radius * 2.0f, radius * 2.0f, radius * 2.0f), position(position), prevPosition(position), color(color) { }

	void movePosition(const glm::vec3& posChange)
	{
		prevPosition = position;
		position += posChange;
		wasMoved = true;
	}

	void handleNoPositionChange()
	{
		if (wasMoved)
		{
			wasMoved = false;
			return;
		}
		prevPosition = position;
	}
};

struct PixelParamData
{
    int uncappedParamNum;
	int paramNum;
	int paramIndexes[40];
};

class GfxInterface
{
public:
	GfxInterface(SceneConfig sceneConfig) : sceneConfig(sceneConfig) { }

	virtual void setSceneConfig(SceneConfig config)
	{
		if (sceneConfig != config)
		{
			sceneConfig = config;
			sceneConfigChanged();
		}
	}

	virtual void handleUserInteractions() { }

	virtual void render(glm::ivec2 resolution, renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> particleData, 
		renderer::fb_ptr canvas = nullptr, renderer::ssbo_ptr<PixelParamData> pixelParamData = nullptr) = 0;

	const std::vector<Obstacle>& getObstacles() const
	{
		return obstacles;
	}

	virtual ~GfxInterface() = default;

protected:
	SceneConfig sceneConfig;

	std::vector<Obstacle> obstacles;

	virtual void sceneConfigChanged() = 0;
	
};

} // namespace vis