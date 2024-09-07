#pragma once

#include "../engine/shaderProgram.h"

namespace renderer
{

class ComputeProgram : public GpuProgram
{
public:
	ComputeProgram(const std::string& computeShaderPath);

	ComputeProgram& operator=(const ComputeProgram&) = delete;
	ComputeProgram(const ComputeProgram&) = delete;
	ComputeProgram& operator=(ComputeProgram&&) = delete;
	ComputeProgram(ComputeProgram&&) = delete;

	/**
	 * \brief Dispatches the compute shader with the specified number of work groups.
	 * \param numWorkGroupsX - The number of work groups in the x direction.
	 * \param numWorkGroupsY - The number of work groups in the y direction.
	 * \param numWorkGroupsZ - The number of work groups in the z direction.
	 */
	void dispatchCompute(GLuint numWorkGroupsX, GLuint numWorkGroupsY, GLuint numWorkGroupsZ) const;

private:
	static bool computeShaderInfoCollected;
	static GLint maxComputeWorkGroupCount[3];
};

}
