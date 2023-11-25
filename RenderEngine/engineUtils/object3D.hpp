#pragma once

#include <glm/glm.hpp>
#include "../engine/renderEngine.h"
#include "../geometries/geometry.h"
#include <memory>


struct Object3D {
	std::shared_ptr<Geometry> geometry;
	unsigned int shaderProgramId;
	glm::mat4 M;
	unsigned int textureId;				//The texture is not deleted in the destructor
	bool hasTexture;
	float textureScale;
	glm::vec3 specularColor;
	float shininess;

	Object3D(std::shared_ptr<Geometry> geometry, unsigned int shaderProgramId, glm::mat4 M, glm::vec3 specularColor, 
		float shininess, bool hasTexture = false, unsigned int textureId = 0, float textureScale = 1.0f) 
		: geometry(geometry), shaderProgramId(shaderProgramId), M(M), textureId(textureId), hasTexture(hasTexture), 
			specularColor(specularColor), shininess(shininess), textureScale(textureScale) { }

	void draw(int num = -1) {
		auto& engine = geometry->engine;
		engine->activateGPUProgram(shaderProgramId);
		engine->setUniformMat4(shaderProgramId, "object.modelMatrix", M);
		if (hasTexture) {
			engine->bindTexture(0, textureId);
			engine->setUniformSampler(shaderProgramId, "material.colorTexture", 0);
			engine->setUniformFloat(shaderProgramId, "material.textureScale", textureScale);
		}
		engine->setUniformVec3(shaderProgramId, "material.specularColor", specularColor);
		engine->setUniformFloat(shaderProgramId, "material.shininess", shininess);
		geometry->draw(num);
	}
};

