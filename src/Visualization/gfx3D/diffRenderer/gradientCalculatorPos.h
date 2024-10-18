#pragma once

#include "gradientCalculatorInterface.h"

namespace visual
{

class GradientCalculatorPos : public GradientCalculatorInterface
{
public:
	GradientCalculatorPos(std::shared_ptr<ParamInterface> renderer, std::shared_ptr<genericfsim::manager::SimulationManager> manager);

	void updateOptimizedFloats(renderer::ssbo_ptr<float> data, renderer::ssbo_ptr<float> particleMovementAbs) override;

	void updateParticleParams(renderer::ssbo_ptr<ParticleShaderData> data) override;

	void reset() override;

	renderer::ssbo_ptr<float> calculateGradient(renderer::fb_ptr referenceFramebuffer) override;

	renderer::ssbo_ptr<float> getFloatParams() override;

	renderer::ssbo_ptr<ParticleShaderData> getParticleData() override;

	size_t getOptimizedParamCountPerParticle() const override;

	void formatFloatParamsPostUpdate(renderer::ssbo_ptr<float> data) const override;

private:
	ParamFloat speedAbsPerturbation = ParamFloat("Speed abs perturbation", 0.2f, 0.0f, 5.0f);
	ParamFloat posPerturbation = ParamFloat("Pos perturbation", 0.05f, 0.0f, 0.5f);
	ParamBool capPositionsToBox = ParamBool("Cap positions to box", true);

	renderer::ssbo_ptr<ParticleShaderData> perturbationPresetSSBO;

	renderer::compute_ptr perturbationProgram;
	renderer::compute_ptr stochaisticColorGradientProgram;
	renderer::compute_ptr stochaisticDepthGradientProgram;
	renderer::compute_ptr particleDataToFloatProgram;
	renderer::compute_ptr floatToParticleDataProgram;
	renderer::compute_ptr floatPosClamperProgram;

	std::pair<glm::vec3, glm::vec3> getBoxBounds() const;
};


} // namespace visual
