#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#include "../engine/shaderProgram.h"
#include "../engine/texture.h"
#include "../geometries/geometry.h"

#include "camera2D.hpp"
#include "camera3D.hpp"
#include "lights.hpp"

#include "../engine/uniforms.hpp"

namespace renderer
{

template<typename R>
class Object : public UniformGatherer
{
public:
	u_var(diffuseColor, glm::vec4);

	u_var(colorTextureScale, float);
	u_var(colorTexture, Texture);

	std::shared_ptr<R> drawable;
	std::shared_ptr<GpuProgram> shaderProgram;

	Object(std::shared_ptr<R> drawable, std::shared_ptr<GpuProgram> program)
		: UniformGatherer("object.", true, diffuseColor, colorTextureScale, colorTexture, modelMatrix, modelMatrixInverse),
		drawable(drawable), shaderProgram(program) 
	{
		updateModelMatrix();
	}

	void setScale(glm::vec3 scale)
	{
		this->scale = scale;
		updateModelMatrix();
	}

	void setPitch(float pitch)
	{
		this->pitch = pitch;
		updateModelMatrix();
	}

	void setYaw(float yaw)
	{
		this->yaw = yaw;
		updateModelMatrix();
	}

	void setRoll(float roll)
	{
		this->roll = roll;
		updateModelMatrix();
	}

	void setPosition(glm::vec4 position)
	{
		this->position = glm::vec3(position.x, position.y, position.z);
		updateModelMatrix();
	}

	/**
	 * \brief The model matrix is overwritten by calling any of the set functions
	 * \param M - The model matrix
	 */
	void setModelMatrix(const glm::mat4& M)
	{
		this->modelMatrix = M;
		this->modelMatrixInverse = glm::inverse(M);
	}

	void draw() const
	{
		if(!shaderProgram)
		{
			throw std::runtime_error("No shader program set for object");
		}
		shaderProgram->activate();
		preDraw();
		setUniforms(*shaderProgram);
		drawable->draw();
	}

protected:
	u_var(modelMatrix, glm::mat4);
	u_var(modelMatrixInverse, glm::mat4);

	glm::vec3 scale = glm::vec3(1.0f);
	glm::vec3 position = glm::vec3(0.0f);
	float pitch = 0.0f;
	float yaw = 0.0f;
	float roll = 0.0f;

	void updateModelMatrix()
	{
		glm::mat4 M = glm::mat4(1.0f);
		M = glm::translate(M, position);
		M = glm::rotate(M, roll, glm::vec3(0.0f, 0.0f, 1.0f));
		M = glm::rotate(M, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
		M = glm::rotate(M, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
		M = glm::scale(M, scale);
		modelMatrix = M;
		modelMatrixInverse = glm::inverse(M);
	}

	virtual void preDraw() const = 0;
};

template<typename R = Drawable>
class Object2D : public Object<R>
{
public:
	Object2D(std::shared_ptr<R> drawable, std::shared_ptr<GpuProgram> program)
		: Object<R>(drawable, program) { }

protected:
	void preDraw() const override { }
};

template<typename R = Drawable>
class Object3D : public Object<R>
{
public:
	Object3D(std::shared_ptr<R> drawable, std::shared_ptr<GpuProgram> program)
		: Object<R>(drawable, program)
	{
		Object<R>::addUniform(specularColor, shininess);
	}

	u_var(specularColor, glm::vec4);
	u_var(shininess, float);

protected:
	void preDraw() const override { }
};


} // namespace renderer
