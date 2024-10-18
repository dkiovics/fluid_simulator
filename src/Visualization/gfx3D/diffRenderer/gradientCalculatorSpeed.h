#pragma once

#include <manager/simulationManager.h>
#include "gradientCalculatorInterface.h"

namespace visual
{

class GradientCalculatorSpeed : public GradientCalculatorInterface
{
public:
	GradientCalculatorSpeed(std::shared_ptr<ParamInterface> renderer, 
		std::shared_ptr<genericfsim::manager::SimulationManager> manager);

	void updateOptimizedFloats(renderer::ssbo_ptr<float> data, renderer::ssbo_ptr<float> particleMovementAbs) override;

	void updateParticleParams(renderer::ssbo_ptr<ParticleShaderData> data) override;

	void reset() override;

	renderer::ssbo_ptr<float> calculateGradient(renderer::fb_ptr referenceFramebuffer) override;

	renderer::ssbo_ptr<float> getFloatParams() override;

	renderer::ssbo_ptr<ParticleShaderData> getParticleData() override;

	size_t getOptimizedParamCountPerParticle() const override;

	void formatFloatParamsPreUpdate(renderer::ssbo_ptr<float> data) const override;

	void formatFloatParamsPostUpdate(renderer::ssbo_ptr<float> data) const override;

private:
	ParamFloat speedPerturbation = ParamFloat("Speed perturbation", 5.0f, 0.0f, 50.0f);
	ParamFloat simulationDt = ParamFloat("Simulation dt", 0.02f, 0.001f, 0.1f);
	ParamInt simulationIterationCount = ParamInt("Simulation iteration count", 2, 1, 10);
	ParamBool resetSpeedsAfterIteration = ParamBool("Reset speeds after iteration", true);
	ParamBool speedCapEnabled = ParamBool("Speed cap enabled", true);
	ParamFloat speedCap = ParamFloat("Speed cap", 10.0f, 0.0f, 100.0f);
	ParamBool simulatorEnabled = ParamBool("Simulator enabled", true);
	ParamBool gravityEnabled = ParamBool("Gravity enabled", true);
	ParamFloat gravityValue = ParamFloat("Gravity value", -250.0f, 0.0f, -300.0f);

	genericfsim::manager::SimulatorCopy simulatorCopy;

	struct ParticleShaderDataSpeed
	{
		glm::vec4 speed;
		static constexpr size_t paramCount = 4;
	};

	renderer::ssbo_ptr<ParticleShaderDataSpeed> perturbationPresetSSBO;
	renderer::ssbo_ptr<float> speedSSBO;
	renderer::ssbo_ptr<float> speedPositiveOffsetSSBO;
	renderer::ssbo_ptr<float> speedNegativeOffsetSSBO;

	renderer::compute_ptr perturbationProgram;
	renderer::compute_ptr stochaisticColorGradientProgram;
	renderer::compute_ptr stochaisticDepthGradientProgram;

	void simulateSpeeds(renderer::ssbo_ptr<float> speeds, bool updateSpeeds = false);
	void updateParticleDataFromSim(renderer::ssbo_ptr<ParticleShaderData> data);
	void updateOptimizedParticleDataFromSim(renderer::ssbo_ptr<float> particleMovementAbs);
	void updateSimFromParticleData(renderer::ssbo_ptr<ParticleShaderData> data);
};

} // namespace visual
