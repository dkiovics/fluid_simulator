#pragma once

#include <glm/glm.hpp>
#include "../engine/renderEngine.h"
#include "../geometries/geometry.h"
#include <memory>
#include <vector>
#include <string>


struct Light {
	glm::vec4 position;		//If the w component is 0 than the light is directional, otherwise it's a spotlight
	glm::vec3 powerDensity;

	Light(glm::vec4 position, glm::vec3 powerDensity) : position(position), powerDensity(powerDensity) { }
};

struct Lights {
	std::vector<Light> lights;
	std::shared_ptr<renderer::RenderEngine> engine;
	std::vector<unsigned int> shaderProgramIds;

	Lights(std::shared_ptr<renderer::RenderEngine> engine, std::vector<unsigned int> shaderProgramIds) : engine(engine), shaderProgramIds(shaderProgramIds) { }

	void setUniforms() {
		for (auto shaderProgram : shaderProgramIds) {
			engine->activateGPUProgram(shaderProgram);
			engine->setUniformInt(shaderProgram, "lightNum", lights.size());
			for (int p = 0; p < lights.size(); p++) {
				engine->setUniformVec4(shaderProgram, "lights[" + std::to_string(p) + "].position", lights[p].position);
				engine->setUniformVec3(shaderProgram, "lights[" + std::to_string(p) + "].powerDensity", lights[p].powerDensity);
			}
		}
	}
};
