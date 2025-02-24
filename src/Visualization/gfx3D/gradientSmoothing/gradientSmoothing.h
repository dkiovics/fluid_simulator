#pragma once

#include <array>
#include "gfx3D/diffRenderer/gradientCalculatorInterface.h"

namespace visual
{

class GradientSmoothing
{
public:
	GradientSmoothing(float smoothingSphereR, glm::vec3 particleBox);

	void smoothGradient(renderer::ssbo_ptr<GradientCalculatorInterface::ParticleGradientData> data);

	const glm::vec3 particleBox;
	const float smoothingSphereR;

private:
	struct ParticleIndexList
	{
		int indexCount;
		std::array<int, 200> indices;
	};

	const glm::ivec3 particleCellCount;

	renderer::ssbo_ptr<ParticleIndexList> particleIndexListSSBO;
	renderer::ssbo_ptr<glm::vec4> particleGradientTmpSSBO;

	renderer::compute_ptr smoothGradientCompute;
	renderer::compute_ptr gradientCopyCompute;

	size_t getParticleIndex(glm::vec3 particlePos) const;
};

} // namespace visual
