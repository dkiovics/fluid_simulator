#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glad.h>
#include "texture.h"

namespace renderer
{

class RenderEngine;

class ShaderProgram
{
public:
	
	class UniformProxy
	{
	public:
		bool operator=(const float value) const;
		bool operator=(const int value) const;
		bool operator=(const glm::vec2& value) const;
		bool operator=(const glm::vec3& value) const;
		bool operator=(const glm::vec4& value) const;
		bool operator=(const glm::mat3& value) const;
		bool operator=(const glm::mat4& value) const;
		bool operator=(const Texture& texture) const;

	private:
		friend class ShaderProgram;
		UniformProxy(const int programId, const std::string& name);
		const int programId;
		const std::string name;
	};

	ShaderProgram& operator=(const ShaderProgram&) = delete;
	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(ShaderProgram&&) = delete;
	ShaderProgram(ShaderProgram&&) = delete;

	/**
	 * Returns a proxy object that can be used to set the value of a uniform variable in the shader program.
	 * 
	 * \param name - The name of the uniform variable.
	 */
	UniformProxy operator[](const std::string& name) const;
	
	ShaderProgram(const std::string& vertexShaderName, const std::string& fragmentShaderName, std::shared_ptr<RenderEngine> engine);

	/**
	 * \brief Makes this the currently active gpu program.
	 */
	void activate() const;

	~ShaderProgram();

private:
	std::shared_ptr<RenderEngine> renderEngine;
	GLuint programId;
};



} // namespace renderer
