#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glad.h>
#include "texture.h"
#include "../compute/computeTexture.h"
#include <spdlog/spdlog.h>

namespace renderer
{

class RenderEngine;

class GpuProgram
{
public:
	
	class UniformProxy
	{
	public:
		bool operator=(const float value) const;
		bool operator=(const int value) const;
		bool operator=(const glm::vec2& value) const;
		bool operator=(const glm::ivec2& value) const;
		bool operator=(const glm::vec3& value) const;
		bool operator=(const glm::vec4& value) const;
		bool operator=(const glm::mat3& value) const;
		bool operator=(const glm::mat4& value) const;
		bool operator=(const Texture& texture) const;

		bool setImageUnit(const ComputeTexture& texture) const;

	private:
		friend class GpuProgram;
		UniformProxy(const int programId, const std::string& name);
		const int programId;
		const std::string name;
	};

	GpuProgram& operator=(const GpuProgram&) = delete;
	GpuProgram(const GpuProgram&) = delete;
	GpuProgram& operator=(GpuProgram&&) = delete;
	GpuProgram(GpuProgram&&) = delete;

	/**
	 * Returns a proxy object that can be used to set the value of a uniform variable in the shader program.
	 * 
	 * \param name - The name of the uniform variable.
	 */
	UniformProxy operator[](const std::string& name) const;

	GpuProgram();

	/**
	 * \brief Makes this the currently active gpu program.
	 */
	void activate() const;

	virtual ~GpuProgram();

protected:
	RenderEngine& renderEngine;
	GLuint programId;
};


class ShaderProgram : public GpuProgram
{
public:
	ShaderProgram(const std::string& vertexShaderName, const std::string& fragmentShaderName);
};


} // namespace renderer
