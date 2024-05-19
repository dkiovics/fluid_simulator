#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>

#include "../engine/shaderProgram.h"
#include "../engine/uniforms.hpp"

namespace renderer
{

struct Light : public UniformGatherer
{
	//If the w component is 0 than the light is directional, otherwise it's a spotlight
	u_var(position, glm::vec4);
	u_var(powerDensity, glm::vec3);

	Light(glm::vec4 position, glm::vec3 powerDensity) 
		: UniformGatherer("", false, this->position, this->powerDensity) 
	{
		this->position = position;
		this->powerDensity = powerDensity;
	}
};

class Lights : public UniformProvider, public UniformGathererGlobal
{
public:
	std::vector<std::unique_ptr<Light>> lights;

	void setUniforms(const renderer::GpuProgram& program, const std::string&) const override {
		program["lightNum"] = (int)lights.size();
		for (int p = 0; p < lights.size(); p++) {
			lights[p]->setUniforms(program, "lights[" + std::to_string(p) + "].");
		}
	}

	void setUniformsForAllPrograms() const
	{
		UniformGathererGlobal::setUniformsForAllPrograms();
	}

protected:
	void setUniformsGlobal(const GpuProgram& program) const override
	{
		setUniforms(program, "");
	}
};

} // namespace renderer


