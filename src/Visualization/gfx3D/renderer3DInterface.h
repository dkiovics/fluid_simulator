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

struct ParticleShaderData
{
	glm::vec4 posAndSpeed;
	glm::vec4 density;
	static constexpr size_t paramCount = 4;
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
	std::shared_ptr<genericfsim::manager::SimulationManager> simManager;

	bool operator==(const ConfigData3D& other) const
	{
		return screenSize == other.screenSize && sceneCenter == other.sceneCenter && boxSize == other.boxSize 
			&& maxParticleSpeed == other.maxParticleSpeed && particleRadius == other.particleRadius 
			&& speedColor == other.speedColor && simManager.get() == other.simManager.get();
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