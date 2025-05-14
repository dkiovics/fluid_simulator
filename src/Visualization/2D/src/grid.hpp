#pragma once
#include "engineUtils/object.h"
#include "geometries/basicGeometries.h"
#include <glm/glm.hpp>

namespace visual
{

class Grid2D : public renderer::Object<renderer::InstancedGeometry>
{
private:
	std::shared_ptr<renderer::InstancedGeometry> verticalLines;
	std::shared_ptr<renderer::InstancedGeometry> horizontalLines;

protected:
	void preDraw() const override { }

public:
	size_t gridWidth;
	size_t gridHeight;

	u_var(simSize, glm::vec2);
	u_var(simCenter, glm::vec2);

	Grid2D(glm::vec2 simSize, glm::vec2 simCenter, size_t gridWidth, size_t gridHeight, float lineWidth)
		: renderer::Object<renderer::InstancedGeometry>(nullptr, nullptr),
		gridWidth(gridWidth), gridHeight(gridHeight)
	{
		addUniform(this->simSize, this->simCenter);
		shaderProgram = std::make_shared<renderer::ShaderProgram>("shaders/visualization/2D/grid.vert", "shaders/visualization/2D/basic.frag");
		verticalLines = std::make_shared<renderer::InstancedGeometry>(std::make_shared<renderer::Line>(glm::vec3(0, -0.5f, 0), glm::vec3(0, 0.5f, 0), lineWidth));
		horizontalLines = std::make_shared<renderer::InstancedGeometry>(std::make_shared<renderer::Line>(glm::vec3(-0.5f, 0, 0), glm::vec3(0.5f, 0, 0), lineWidth));
		this->simSize = simSize;
		this->simCenter = simCenter;
	}

	void draw() const override
	{
		if (!shaderProgram)
		{
			throw std::runtime_error("No shader program set for object");
		}
		shaderProgram->activate();
		preDraw();
		setUniforms(*shaderProgram);

		(*shaderProgram)["increment"] = glm::vec3(0.0f, (*simSize).y / gridHeight, 0.0f);
		horizontalLines->setInstanceNum(gridHeight + 1);
		horizontalLines->draw();

		(*shaderProgram)["increment"] = glm::vec3((*simSize).x / gridWidth, 0.0f, 0.0f);
		verticalLines->setInstanceNum(gridWidth + 1);
		verticalLines->draw();
	}

};

} // namespace visual
