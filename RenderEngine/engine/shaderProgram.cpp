#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include "shaderProgram.h"
#include "renderEngine.h"

using namespace renderer;

constexpr bool showWarnings = false;

static inline int uniformWarning(int program, const char* name)
{
	int location = glGetUniformLocation(program, name);
	if (location == -1 && showWarnings)
		std::cout << "WARNING: could not find uniform named " << name << std::endl;
	return location;
}

bool renderer::ShaderProgram::UniformProxy::operator=(const float value) const
{
	int val = uniformWarning(programId, name.c_str());
	if(val == -1)
		return false;
	glUniform1f(val, value);
	return true;
}

bool renderer::ShaderProgram::UniformProxy::operator=(const int value) const
{
	int val = uniformWarning(programId, name.c_str());
	if (val == -1)
		return false;
	glUniform1i(val, value);
	return true;
}

bool renderer::ShaderProgram::UniformProxy::operator=(const glm::vec2& value) const
{
	int val = uniformWarning(programId, name.c_str());
	if (val == -1)
		return false;
	glUniform2fv(val, 1, &value[0]);
	return true;
}

bool renderer::ShaderProgram::UniformProxy::operator=(const glm::vec3& value) const
{
	int val = uniformWarning(programId, name.c_str());
	if (val == -1)
		return false;
	glUniform3fv(val, 1, &value[0]);
	return true;
}

bool renderer::ShaderProgram::UniformProxy::operator=(const glm::vec4& value) const
{
	int val = uniformWarning(programId, name.c_str());
	if (val == -1)
		return false;
	glUniform4fv(val, 1, &value[0]);
	return true;
}

bool renderer::ShaderProgram::UniformProxy::operator=(const glm::mat3& value) const
{
	int val = uniformWarning(programId, name.c_str());
	if (val == -1)
		return false;
	glUniformMatrix3fv(val, 1, GL_FALSE, glm::value_ptr(value));
	return true;
}

bool renderer::ShaderProgram::UniformProxy::operator=(const glm::mat4& value) const
{
	int val = uniformWarning(programId, name.c_str());
	if (val == -1)
		return false;
	glUniformMatrix4fv(val, 1, GL_FALSE, glm::value_ptr(value));
	return true;
}

bool renderer::ShaderProgram::UniformProxy::operator=(const Texture& value) const
{
	int val = uniformWarning(programId, name.c_str());
	if (val == -1)
		return false;
	glUniform1i(val, value.getTexSampler());
	return true;
}

renderer::ShaderProgram::UniformProxy::UniformProxy(const int programId, const std::string& name) : programId(programId), name(name) {}

renderer::ShaderProgram::UniformProxy renderer::ShaderProgram::operator[](const std::string& name) const
{
	activate();
	return UniformProxy(programId, name);
}

void renderer::ShaderProgram::activate() const
{
	renderEngine->activateGPUProgram(programId);
}

renderer::ShaderProgram::~ShaderProgram()
{
	glDeleteProgram(programId);
}

renderer::ShaderProgram::ShaderProgram(const std::string& vertexShaderName, const std::string& fragmentShaderName, std::shared_ptr<RenderEngine> engine)
	: renderEngine(engine)
{
	std::ifstream vertexShaderFile(vertexShaderName);
	if (!vertexShaderFile)
		throw std::runtime_error("Vertex shader not found");
	std::ostringstream vertexStream;
	vertexStream << vertexShaderFile.rdbuf();
	std::string vertexShaderSource = vertexStream.str();
	const char* tmp = vertexShaderSource.c_str();
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &tmp, NULL);
	glCompileShader(vertexShader);
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		throw std::runtime_error(std::string("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n") + infoLog);
	}

	std::ifstream fragmentShaderFile(fragmentShaderName);
	if (!fragmentShaderFile)
		throw std::runtime_error("Fragment shader not found");
	std::ostringstream fragmentStream;
	fragmentStream << fragmentShaderFile.rdbuf();
	std::string fragmentShaderSource = fragmentStream.str();
	tmp = fragmentShaderSource.c_str();
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &tmp, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		throw std::runtime_error(std::string("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n") + infoLog);
	}

	programId = glCreateProgram();
	glAttachShader(programId, vertexShader);
	glAttachShader(programId, fragmentShader);
	glLinkProgram(programId);
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, 512, NULL, infoLog);
		throw std::runtime_error(std::string("ERROR::SHADER::PROGRAM::LINKING_FAILED\n") + infoLog);
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}
