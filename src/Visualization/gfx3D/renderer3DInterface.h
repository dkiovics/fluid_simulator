#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <engine/framebuffer.h>
#include <param.hpp>
#include <manager/simulationManager.h>
#include <compute/storageBuffer.h>

namespace visual
{

struct Gfx3DRenderData
{
	std::vector<genericfsim::manager::SimulationManager::ParticleGfxData> particleData;

	Gfx3DRenderData(std::vector<genericfsim::manager::SimulationManager::ParticleGfxData>&& data)
		: particleData(std::move(data))
	{
	}
	Gfx3DRenderData() { }
	Gfx3DRenderData(const Gfx3DRenderData& other) = default;
	Gfx3DRenderData(Gfx3DRenderData&& other) = default;
	Gfx3DRenderData& operator=(const Gfx3DRenderData& other) = default;
	Gfx3DRenderData& operator=(Gfx3DRenderData&& other) = default;
};

struct ParticleShaderData
{
	glm::vec4 posAndSpeed;
	glm::vec4 density;
};

struct ConfigData3D
{
	glm::ivec2 screenSize;
	glm::vec3 sceneCenter;
	glm::vec3 boxSize;
	float maxParticleSpeed;
	float particleRadius;
	bool speedColorEnabled;
	glm::vec3 color;
	glm::vec3 speedColor;

	bool operator==(const ConfigData3D& other) const
	{
		return screenSize == other.screenSize && sceneCenter == other.sceneCenter && boxSize == other.boxSize 
			&& maxParticleSpeed == other.maxParticleSpeed && particleRadius == other.particleRadius 
			&& speedColor == other.speedColor;
	}
};

class Renderer3DInterface : public ParamLineCollection
{
public:
	using ParamLineCollection::ParamLineCollection;

	virtual void render(renderer::fb_ptr framebuffer, renderer::ssbo_ptr<ParticleShaderData> data) = 0;

	virtual void setConfigData(const ConfigData3D& config) = 0;
};

} // namespace visual