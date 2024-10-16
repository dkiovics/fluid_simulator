#pragma once

#include "gradientCalculatorInterface.h"

namespace visual
{

class GradientCalculatorPos : public GradientCalculatorInterface
{
public:
	GradientCalculatorPos(std::shared_ptr<ParamInterface> renderer);

	void updateOptimizedFloats(renderer::ssbo_ptr<float> data, renderer::ssbo_ptr<float> particleMovementAbs) override;

	void updateParticleParams(renderer::ssbo_ptr<ParticleShaderData> data) override;

	void reset() override;

	renderer::ssbo_ptr<float> calculateGradient(renderer::fb_ptr referenceFramebuffer) override;

	renderer::ssbo_ptr<float> getFloatParams() override;

	renderer::ssbo_ptr<ParticleShaderData> getParticleData() override;

	size_t getOptimizedParamCountPerParticle() const override;

private:
	renderer::fb_ptr pertPlusFramebuffer;
	renderer::fb_ptr pertMinusFramebuffer;

	renderer::ssbo_ptr<ParticleShaderData> perturbationPresetSSBO;
	renderer::ssbo_ptr<ParticleShaderData> paramNegativeOffsetSSBO;
	renderer::ssbo_ptr<ParticleShaderData> paramPositiveOffsetSSBO;
	renderer::ssbo_ptr<ParticleShaderData> optimizedParamsSSBO;

	renderer::ssbo_ptr<float> stochaisticGradientSSBO; 

	renderer::compute_ptr perturbationProgram;
	renderer::compute_ptr stochaisticColorGradientProgram;
	renderer::compute_ptr stochaisticDepthGradientProgram;
	renderer::compute_ptr particleDataToFloatProgram;
	renderer::compute_ptr floatToParticleDataProgram;
};


} // namespace visual
