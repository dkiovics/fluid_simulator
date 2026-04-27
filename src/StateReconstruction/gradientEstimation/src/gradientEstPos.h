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

    renderer::compute_ptr perturbationProgram;
    renderer::compute_ptr stochaisticColorGradientProgram;
    renderer::compute_ptr stochaisticDepthGradientProgram;
};

} // namespace diffrender
