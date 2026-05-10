#pragma once

#include <glm/glm.hpp>
#include "engine/framebuffer.h"
#include "compute/storageBuffer.h"
#include "manager/simulationManager.h"


namespace visual
{

class PixelParamBuffers
{
public:
    // Worst-case slots per pixel in `xIndex`. X filter radius is clamped to <=35
    // in bilateral/gaussian, so up to 71 samples per pixel. 80 leaves headroom.
    static constexpr unsigned int INDEX_BUFFER_MULTIPLIER = 25;

    PixelParamBuffers(glm::ivec2 screenSize)
        : screenSize(0, 0)
    {
        xCount = renderer::make_ssbo<uint32_t>(1, GL_DYNAMIC_COPY);
        xOffset = renderer::make_ssbo<uint32_t>(1, GL_DYNAMIC_COPY);
        xIndex = renderer::make_ssbo<int32_t>(1, GL_DYNAMIC_COPY);
        yRadius = renderer::make_ssbo<int32_t>(1, GL_DYNAMIC_COPY);
        resize(screenSize);
    }

    void resize(glm::ivec2 newSize)
    {
        if (newSize == screenSize)
            return;
        screenSize = newSize;
        const unsigned int pixels = (unsigned int) (screenSize.x * screenSize.y);
        xCount->setSize(pixels);
        xOffset->setSize(pixels);
        xIndex->setSize(pixels * INDEX_BUFFER_MULTIPLIER);
        yRadius->setSize(pixels);
    }

    glm::ivec2 getScreenSize() const { return screenSize; }
    unsigned int getPixelCount() const { return (unsigned int) (screenSize.x * screenSize.y); }

    renderer::ssbo_ptr<uint32_t> xCount;
    renderer::ssbo_ptr<uint32_t> xOffset;
    renderer::ssbo_ptr<int32_t>  xIndex;
    renderer::ssbo_ptr<int32_t>  yRadius;

private:
    glm::ivec2 screenSize;
};

using pixel_params_ptr = std::shared_ptr<PixelParamBuffers>;

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
		renderer::fb_ptr canvas = nullptr, pixel_params_ptr pixelParamBuffers = nullptr) = 0;

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