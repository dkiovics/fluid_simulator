#include "gradientEstPos.h"

using namespace diffrender;

diffrender::GradientEstPos::GradientEstPos(glm::ivec2 resolution, std::shared_ptr<visual::Visuals3D> visuals3D)
    : GradientEstimation(resolution, visuals3D)
{
    perturbationProgram = renderer::make_compute("shaders/stateReconstruction/gradientEstimation/perturbation.comp");
    stochaisticColorGradientProgram =
        renderer::make_compute("shaders/stateReconstruction/gradientEstimation/stochGradient_color.comp");
    stochaisticDepthGradientProgram =
        renderer::make_compute("shaders/stateReconstruction/gradientEstimation/stochGradient_depth.comp");

    auto camera = visuals3D->getCamera();
    camera->addProgram({ stochaisticDepthGradientProgram });
    camera->setUniformsForAllPrograms();
}

void diffrender::GradientEstPos::startGradientEstimation(
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> particleData,
    std::vector<ReferenceData> referenceData)
{
    GradientEstimation::startGradientEstimation(particleData, referenceData);

    auto& registry = controls::ControlRegistry::getInstance();
    const float posPerturbation = registry["state.pos_perturbation_amount"];

    bool updateSSBO = false;
    if (!perturbationPresetSSBO)
    {
        perturbationPresetSSBO =
            renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(particleData->getSize(), GL_STATIC_COPY);
        updateSSBO = true;
    }
    else if (perturbationPresetSSBO->getSize() != particleData->getSize())
    {
        perturbationPresetSSBO->setSize(particleData->getSize());
        updateSSBO = true;
    }
    if (updateSSBO)
    {
        perturbationPresetSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        for (int i = 0; i < particleData->getSize(); i++)
        {
            (*perturbationPresetSSBO)[i].posAndDensity =
                glm::vec4(posPerturbation, posPerturbation, posPerturbation, 0.0f);
            (*perturbationPresetSSBO)[i].velocity = glm::vec4(0.0f);
        }

        perturbationPresetSSBO->unmapBuffer();
    }

    // Per-view texture sampler bindings are set inside executeGradientEstimationStep,
    // since we now dispatch the gradient compute once per view.
}

bool diffrender::GradientEstPos::executeGradientEstimationStep()
{
    if (!gradientEstimationInProgress)
        throw std::runtime_error(
            "GradientEstimation::executeGradientEstimationStep: Gradient estimation is not in progress");

    auto& registry = controls::ControlRegistry::getInstance();
    const bool useDepthImage = registry["state.use_depth_image"];
    const float depthErrorScale = registry["state.depth_error_scale"];

    (*perturbationProgram)["seed"] = std::rand() % 10000;
    (*perturbationProgram)["boxLowerBound"] = fluidBoxbounds.first;
    (*perturbationProgram)["boxUpperBound"] = fluidBoxbounds.second;
    (*perturbationProgram)["posClampEnabled"] = true;
    optimizedParamsSSBO->bindBuffer(0);
    perturbationPresetSSBO->bindBuffer(1);
    paramNegativeOffsetSSBO->bindBuffer(2);
    paramPositiveOffsetSSBO->bindBuffer(3);
    perturbationProgram->dispatchCompute(optimizedParamsSSBO->getSize() / 64 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    for (size_t i = 0; i < referenceData.size(); i++)
    {
        bindPerturbedRenderedSceneTextures(i);

        visuals3D->getCamera()->setCameraData(referenceData[i].cameraData);

        visuals3D->render(pertPlusFramebuffer->getSize(), paramPositiveOffsetSSBO, pertPlusFramebuffer);
        visuals3D->render(pertMinusFramebuffer->getSize(), paramNegativeOffsetSSBO, pertMinusFramebuffer);
    }

    paramNegativeOffsetSSBO->bindBuffer(0);
    paramPositiveOffsetSSBO->bindBuffer(1);
    stochaisticGradientSSBO->bindBuffer(2);

    auto& gradientProgram = useDepthImage ? stochaisticDepthGradientProgram : stochaisticColorGradientProgram;
    if (useDepthImage)
        (*gradientProgram)["depthErrorScale"] = depthErrorScale;
    (*gradientProgram)["screenSize"] = screenResolution;

    for (size_t p = 0; p < referenceData.size(); p++)
    {
        const auto& params = pixelParamsPerView[p];
        params->xCount->bindBuffer(20);
        params->xOffset->bindBuffer(21);
        params->xIndex->bindBuffer(22);
        params->yRadius->bindBuffer(23);

        if (useDepthImage)
        {
            (*gradientProgram)["referenceImage"] = *referenceData[p].depthTexture;
            (*gradientProgram)["plusPertImage"] = *perturbedRenderedScenes[p].first.depth;
            (*gradientProgram)["minusPertImage"] = *perturbedRenderedScenes[p].second.depth;
        }
        else
        {
            (*gradientProgram)["referenceImage"] = *referenceData[p].colorTexture;
            (*gradientProgram)["plusPertImage"] = *perturbedRenderedScenes[p].first.color;
            (*gradientProgram)["minusPertImage"] = *perturbedRenderedScenes[p].second.color;
        }

        // stochGradient_{color,depth}.comp uses local_size 16x16 — round up the dispatch
        // to cover the full screen; the shaders early-return on out-of-bounds threads.
        gradientProgram->dispatchCompute(
            (screenResolution.x + 15) / 16,
            (screenResolution.y + 15) / 16,
            1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    gradientSampleCount++;
    if (gradientSampleCount == gradientSampleNum)
    {
        gradientEstimationInProgress = false;
        correctGradient();
        return true;
    }
    return false;
}
