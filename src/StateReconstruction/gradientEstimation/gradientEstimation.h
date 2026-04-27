#pragma once

#include <glm/glm.hpp>
#include <memory>
#include "3D/visuals3D.h"
#include "compute/computeProgram.h"
#include "compute/storageBuffer.h"
#include "engine/framebuffer.h"
#include "engineUtils/camera3D.hpp"
#include "stateReconstructionController.h"

namespace diffrender
{

class GradientEstimation
{
public:
    GradientEstimation(glm::ivec2 resolution, std::shared_ptr<visual::Visuals3D> visuals3D);

    virtual void startGradientEstimation(renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> particleData,
                                         std::vector<ReferenceData> referenceData);

    bool isGradientEstimationInProgress() const;

    virtual bool executeGradientEstimationStep() = 0;

    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> getGradientEstimationResult() const;

    void setFluidBoxBounds(glm::vec3 lowerCorner, glm::vec3 upperCorner)
    {
        fluidBoxbounds = { lowerCorner, upperCorner };
    }

    void handleResolutionChanged(glm::ivec2 size);

protected:
    std::shared_ptr<visual::Visuals3D> visuals3D;

    std::pair<glm::vec3, glm::vec3> fluidBoxbounds;

    std::vector<ReferenceData> referenceData;

    struct RenderedScene
    {
        renderer::render_target_ptr color;
        renderer::render_target_ptr depth;

        RenderedScene(uint32_t width, uint32_t height);

        void resize(uint32_t width, uint32_t height) const;
    };

    std::vector<std::pair<RenderedScene, RenderedScene>> perturbedRenderedScenes;

    renderer::fb_ptr pertPlusFramebuffer;
    renderer::fb_ptr pertMinusFramebuffer;

    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> paramNegativeOffsetSSBO;
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> paramPositiveOffsetSSBO;
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> optimizedParamsSSBO;
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> stochaisticGradientSSBO;

    std::vector<renderer::ssbo_ptr<visual::PixelParamData>> pixelParamsPerView;

    renderer::compute_ptr gradientMultCompute;

    glm::ivec2 screenResolution;
    bool gradientEstimationInProgress = false;
    int gradientSampleNum = 0;
    int gradientSampleCount = 0;

    void correctGradient() const;

    void bindPerturbedRenderedSceneTextures(uint32_t index) const;

    void logPerPixelParamStats() const;

    void initPerViewData(int num);
};

} // namespace diffrender
