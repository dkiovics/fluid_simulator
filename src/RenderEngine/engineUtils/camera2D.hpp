#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <algorithm>
#include <vector>

#include "../engine/shaderProgram.h"
#include "../engine/uniforms.hpp"


namespace renderer
{

class Camera2D : public UniformGatherer, public UniformGathererGlobal
{
private:
	glm::vec2 initialPosition;
	glm::vec2 windowSize;
	float magnification = 1.0f;
	u_var(VP, glm::mat4);

public:
	/**
	 * \brief Constructor for the Camera2D class
	 * \param windowSize - The size of the camera window in world units
	 * \param position - The initial position of the camera (center of the window)
	 */
	Camera2D(const glm::vec2& windowSize, const glm::vec2& position)
		: UniformGatherer("camera.", true, VP), windowSize(windowSize), initialPosition(position)
	{
		init(windowSize, position);
	}

	/**
	 * \brief Initializes the camera with a given window size and position
	 * \param windowSize - The size of the camera window in world units
	 * \param position - The initial position of the camera (center of the window)
	 */
	void init(const glm::vec2& windowSize, const glm::vec2& position)
	{
		this->windowSize = windowSize;
		initialPosition = position;
		magnification = 1.0f;
		glm::mat4 VP = glm::mat4(1);
		VP = glm::scale(VP, glm::vec3(2.0f / windowSize.x * magnification, 2.0f / windowSize.y * magnification, 1));
		VP = glm::translate(VP, -glm::vec3(initialPosition, 0));
		this->VP = VP;
		setUniformsForAllPrograms();
	}

	/**
	 * \brief Magnifies the camera at a given position
	 * \param magPos - The position to magnify at
	 * \param amount - The amount to magnify by
	 */
	void magnify(const glm::vec2& magPos, float amount)
	{
		glm::vec3 magWorldPos = glm::vec3(getWorldPosition(magPos), 0);
		magnification += amount;
		if (magnification < 1)
			magnification = 1;

		glm::vec2 minPos = windowSize * 0.5f / magnification - glm::vec2(0.01f, 0.01f);
		glm::vec2 maxPos = windowSize - windowSize * 0.5f / magnification + glm::vec2(0.01f, 0.01f);

		magWorldPos.x = std::clamp(magWorldPos.x, minPos.x, maxPos.x);
		magWorldPos.y = std::clamp(magWorldPos.y, minPos.y, maxPos.y);
		glm::mat4 VP = glm::mat4(1);
		VP = glm::scale(VP, glm::vec3(2.0f / windowSize.x * magnification, 2.0f / windowSize.y * magnification, 1));
		VP = glm::translate(VP, -magWorldPos);
		this->VP = VP;
		setUniformsForAllPrograms();
	}

	/**
	 * \brief Returns the world position of a given NDC position
	 * \param ndcPos - The NDC position
	 * \return - The world position, based on the camera's position, magnification and window size
	 */
	glm::vec2 getWorldPosition(const glm::vec2& ndcPos) const
	{
		glm::vec4 worldPos = glm::inverse(*VP) * glm::vec4(ndcPos, 0, 1);
		return glm::vec2(worldPos.x, worldPos.y);
	}

	/**
	 * \brief Returns the NDC position of a given world position
	 * \return The NDC position, based on the camera's position, magnification and window size
	 */
	glm::mat4 getVP() const
	{
		return *VP;
	}

protected:
	void setUniformsGlobal(const ShaderProgram& program) const override
	{
		setUniforms(program);
	}

};

} // namespace renderer
