#include "computeProgram.h"
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>

using namespace renderer;

bool ComputeProgram::computeShaderInfoCollected = false;
GLint ComputeProgram::maxComputeWorkGroupCount[3] = { 0, 0, 0 };

renderer::ComputeProgram::ComputeProgram(const std::string& computeShaderPath)
{
    if (!computeShaderInfoCollected)
    {
        GLint maxComputeWorkGroupSize[3];
        GLint maxComputeWorkGroupInvocations;
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxComputeWorkGroupCount[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxComputeWorkGroupCount[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxComputeWorkGroupCount[2]);
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxComputeWorkGroupInvocations);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxComputeWorkGroupSize[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxComputeWorkGroupSize[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxComputeWorkGroupSize[2]);
		computeShaderInfoCollected = true;

        spdlog::info("Compute shader info: \n" + 
            std::string("\tMax work group count: ") + std::to_string(maxComputeWorkGroupCount[0]) + ", " + std::to_string(maxComputeWorkGroupCount[1]) + ", " + std::to_string(maxComputeWorkGroupCount[2]) + "\n" +
			std::string("\tMax work group invocations: ") + std::to_string(maxComputeWorkGroupInvocations) + "\n" +
			std::string("\tMax work group size: ") + std::to_string(maxComputeWorkGroupSize[0]) + ", " + std::to_string(maxComputeWorkGroupSize[1]) + ", " + std::to_string(maxComputeWorkGroupSize[2]));
	}

    // 1. retrieve the vertex/fragment source code from filePath
    std::string computeCode;
    std::ifstream cShaderFile;
    // ensure ifstream objects can throw exceptions:
    cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        cShaderFile.open(computeShaderPath);

        std::stringstream cShaderStream;
        // read file's buffer contents into streams
        cShaderStream << cShaderFile.rdbuf();
        // close file handlers
        cShaderFile.close();
        // convert stream into string
        computeCode = cShaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        spdlog::error("Compute shader source: {} failed to read file", computeShaderPath);
        throw std::runtime_error(std::string("ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ\n") + e.what());
    }
    const char* cShaderCode = computeCode.c_str();
    // 2. compile shaders
    unsigned int compute;
    // compute shader
    compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cShaderCode, NULL);
    glCompileShader(compute);
    int success;
    char infoLog[512];
    glGetShaderiv(compute, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(compute, 512, NULL, infoLog);
        spdlog::error("Compute shader source: {} compilation failed with error: {}", computeShaderPath, infoLog);
        throw std::runtime_error(std::string("ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n") + infoLog);
    }

    // shader Program
    programId = glCreateProgram();
    glAttachShader(programId, compute);
    glLinkProgram(programId);
    glGetShaderiv(programId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(programId, 512, NULL, infoLog);
        spdlog::error("Compute shader program: {} compilation failed with error: {}", computeShaderPath, infoLog);
        throw std::runtime_error(std::string("ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n") + infoLog);
    }
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(compute);
}

void renderer::ComputeProgram::dispatchCompute(GLuint numWorkGroupsX, GLuint numWorkGroupsY, GLuint numWorkGroupsZ) const
{
    if(numWorkGroupsX > maxComputeWorkGroupCount[0] || numWorkGroupsY > maxComputeWorkGroupCount[1] || numWorkGroupsZ > maxComputeWorkGroupCount[2])
		throw std::runtime_error("Number of work groups exceeds the maximum allowed by the hardware");
	activate();
	glDispatchCompute(numWorkGroupsX, numWorkGroupsY, numWorkGroupsZ);
}
