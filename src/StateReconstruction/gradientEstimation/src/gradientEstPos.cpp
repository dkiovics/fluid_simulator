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
    std::vector<ReferenceData> referenceData,
    renderer::fb_ptr currentStateFb,
    int currentCameraPosIdx)
{
    GradientEstimation::startGradientEstimation(particleData, referenceData, currentStateFb, currentCameraPosIdx);

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
    }

    std::vector<std::shared_ptr<renderer::Texture>> plusColor;
    std::vector<std::shared_ptr<renderer::Texture>> minusColor;
    std::vector<std::shared_ptr<renderer::Texture>> plusDepth;
    std::vector<std::shared_ptr<renderer::Texture>> minusDepth;
    for (size_t i = 0; i < referenceData.size(); i++)
    {
        plusColor.push_back(perturbedRenderedScenes[i].first.color);
        minusColor.push_back(perturbedRenderedScenes[i].second.color);
        plusDepth.push_back(perturbedRenderedScenes[i].first.depth);
        minusDepth.push_back(perturbedRenderedScenes[i].second.depth);
    }

    (*stochaisticColorGradientProgram)["plusPertImage"] = plusColor;
    (*stochaisticColorGradientProgram)["minusPertImage"] = minusColor;
    (*stochaisticDepthGradientProgram)["plusPertImage"] = plusDepth;
    (*stochaisticDepthGradientProgram)["minusPertImage"] = minusDepth;
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

        visuals3D->render(paramPositiveOffsetSSBO, true, pertPlusFramebuffer);
        visuals3D->render(paramNegativeOffsetSSBO, true, pertMinusFramebuffer);
    }

    paramNegativeOffsetSSBO->bindBuffer(0);
    paramPositiveOffsetSSBO->bindBuffer(1);
    stochaisticGradientSSBO->bindBuffer(2);
    for (int p = 0; p < referenceData.size(); p++)
    {
        pixelParamsPerView[p]->bindBuffer(3 + p);
    }
    if (useDepthImage)
    {
        std::vector<std::shared_ptr<renderer::Texture>> depthImages;
        for (size_t i = 0; i < referenceData.size(); i++)
        {
            depthImages.push_back(referenceData[i].depthTexture);
        }
        (*stochaisticDepthGradientProgram)["referenceImage"] = depthImages;
        (*stochaisticDepthGradientProgram)["referenceImageNum"] = (int) referenceData.size();
        (*stochaisticDepthGradientProgram)["screenSize"] = screenResolution;
        (*stochaisticDepthGradientProgram)["depthErrorScale"] = depthErrorScale;
        stochaisticDepthGradientProgram->dispatchCompute(referenceData[0].colorTexture->getSize().x,
                                                         referenceData[0].colorTexture->getSize().y, 1);
    }
    else
    {
        std::vector<std::shared_ptr<renderer::Texture>> colorImages;
        for (size_t i = 0; i < referenceData.size(); i++)
        {
            colorImages.push_back(referenceData[i].colorTexture);
        }
        (*stochaisticColorGradientProgram)["referenceImage"] = colorImages;
        (*stochaisticColorGradientProgram)["referenceImageNum"] = (int) referenceData.size();
        (*stochaisticColorGradientProgram)["screenSize"] = referenceData[0].colorTexture->getSize();
        stochaisticColorGradientProgram->dispatchCompute(referenceData[0].colorTexture->getSize().x,
                                                         referenceData[0].colorTexture->getSize().y, 1);
    }
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    gradientSampleCount++;
    if (gradientSampleCount == gradientSampleNum)
    {
        gradientEstimationInProgress = false;
        correctGradient();
        return true;
    }
    return false;
}
