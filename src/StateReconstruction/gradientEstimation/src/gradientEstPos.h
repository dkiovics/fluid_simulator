#pragma once

#include "gradientEstimation/gradientEstimation.h"

namespace diffrender
{

class GradientEstPos : public GradientEstimation
{
public:
    GradientEstPos(glm::ivec2 resolution, std::shared_ptr<visual::Visuals3D> visuals3D);

    void startGradientEstimation(renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> particleData,
                                 std::vector<ReferenceData> referenceData) override;

    bool executeGradientEstimationStep() override;

private:
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> perturbationPresetSSBO;

    // Per-step scalar (positiveError - negativeError) sum, one float per particle.
    // Filled by stochGradient_*.comp across all views, then folded into the long-lived
    // gradient buffer by errorToGradientProgram once per step. Halves to one-eighth the
    // atomicAdds of writing into gradient[] directly.
    renderer::ssbo_ptr<float> errorSumSSBO;

    renderer::compute_ptr perturbationProgram;
    renderer::compute_ptr stochaisticColorGradientProgram;
    renderer::compute_ptr stochaisticDepthGradientProgram;
    renderer::compute_ptr errorToGradientProgram;
};

} // namespace diffrender
