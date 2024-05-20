#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "engine/framebuffer.h"
#include "../../ui/param.hpp"

namespace gfx3D
{

struct Gfx3DRenderData
{
	std::vector<glm::vec3> positions;
	bool positionsChanged = true;
	std::vector<float> speeds;
	glm::vec3 color;
	glm::vec3 speedColor;
	bool speedOrColorChanged = true;

	Gfx3DRenderData(glm::vec3 color, glm::vec3 speedColor)
		: color(color), speedColor(speedColor)
	{
	}
};

struct ConfigData3D
{
	glm::ivec2 screenSize;
	glm::vec3 sceneCenter;
	glm::vec3 boxSize;
	float maxParticleSpeed;
	float particleRadius;
	bool speedColor;
};

class Renderer3DInterface : public ParamLineCollection
{
public:
	using ParamLineCollection::ParamLineCollection;

	virtual void render(std::shared_ptr<renderer::Framebuffer> framebuffer, const Gfx3DRenderData& data) const = 0;

	virtual void setConfigData(const ConfigData3D& config) = 0;
};

} // namespace gfx3D