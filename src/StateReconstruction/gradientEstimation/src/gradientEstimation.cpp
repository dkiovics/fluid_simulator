#include "gradientEstimation/gradientEstimation.h"
#include "controlRegistry.h"

using namespace diffrender;

GradientEstimation::GradientEstimation(glm::ivec2 resolution, std::shared_ptr<visual::Visuals3D> visuals3D)
    : screenResolution(resolution), visuals3D(visuals3D)
{
    gradientMultCompute = renderer::make_compute("shaders/stateReconstruction/gradientEstimation/gradientMult.comp");
    initPerViewData(1);

    paramNegativeOffsetSSBO = renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(1, GL_DYNAMIC_COPY);
    paramPositiveOffsetSSBO = renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(1, GL_DYNAMIC_COPY);
    optimizedParamsSSBO = renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(1, GL_DYNAMIC_COPY);
    stochaisticGradientSSBO = renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(1, GL_DYNAMIC_COPY);

    pertPlusFramebuffer =
        renderer::make_fb(renderer::Framebuffer::toArray({ perturbedRenderedScenes[0].first.color }), nullptr, false);

    pertMinusFramebuffer = std::make_shared<renderer::Framebuffer>(
        renderer::Framebuffer::toArray({ perturbedRenderedScenes[0].second.color }), nullptr, false);
}

void GradientEstimation::startGradientEstimation(
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> particleData,
    std::vector<ReferenceData> referenceData,
    renderer::fb_ptr currentStateFb,
    int currentCameraPosIdx)
{
    auto& registry = controls::ControlRegistry::getInstance();

    this->referenceData = referenceData;

    int dataSize = particleData->getSize();
    if (paramNegativeOffsetSSBO->getSize() != dataSize)
    {
        paramNegativeOffsetSSBO->setSize(dataSize);
        paramPositiveOffsetSSBO->setSize(dataSize);
        optimizedParamsSSBO->setSize(dataSize);
        stochaisticGradientSSBO->setSize(dataSize);
    }
    stochaisticGradientSSBO->fillWithZeros();

    initPerViewData(referenceData.size());

    gradientSampleNum = (int) registry["state.gradient_sample_count"];
    gradientSampleCount = 0;
    gradientEstimationInProgress = true;

    for (int i = 0; i < referenceData.size(); i++)
    {
        visuals3D->getCamera()->setCameraData(referenceData[i].cameraData);
        if (i == currentCameraPosIdx)
            visuals3D->render(particleData, true, currentStateFb, pixelParamsPerView[i]);
        else
            visuals3D->render(particleData, false, nullptr, pixelParamsPerView[i]);
    }
}

bool GradientEstimation::isGradientEstimationInProgress() const
{
    return gradientEstimationInProgress;
}

renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> GradientEstimation::getGradientEstimationResult() const
{
    if (!stochaisticGradientSSBO || gradientSampleCount < gradientSampleNum)
        return nullptr;
    return stochaisticGradientSSBO;
}

void GradientEstimation::resolutionChanged(uint32_t width, uint32_t height)
{
    screenResolution = glm::ivec2(width, height);
    for (auto& scene : perturbedRenderedScenes)
    {
        scene.first.resize(width, height);
        scene.second.resize(width, height);
    }

    for (auto& params : pixelParamsPerView)
    {
        params->setSize(width * height);
    }
    gradientEstimationInProgress = false;
}

GradientEstimation::RenderedScene::RenderedScene(uint32_t width, uint32_t height)
{
    color = renderer::make_render_target(width, height, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    depth = renderer::make_render_target(width, height, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F,
                                         GL_DEPTH_COMPONENT, GL_FLOAT);
}

void GradientEstimation::RenderedScene::resize(uint32_t width, uint32_t height) const
{
    color->resizeTexture(width, height);
    depth->resizeTexture(width, height);
}

void GradientEstimation::correctGradient() const
{
    (*gradientMultCompute)["gradientMult"] = 1.0f / gradientSampleCount;
    stochaisticGradientSSBO->bindBuffer(0);
    gradientMultCompute->dispatchCompute(stochaisticGradientSSBO->getSize() / 64 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GradientEstimation::bindPerturbedRenderedSceneTextures(uint32_t index) const
{
    if (index >= perturbedRenderedScenes.size())
        throw std::runtime_error(
            "GradientCalculatorInterface::bindPerturbedRenderedSceneTextures: index is out of bounds");
    pertMinusFramebuffer->setColorAttachments(
        renderer::Framebuffer::toArray({ perturbedRenderedScenes[index].second.color }));
    pertMinusFramebuffer->setDepthAttachment(perturbedRenderedScenes[index].second.depth);
    pertPlusFramebuffer->setColorAttachments(
        renderer::Framebuffer::toArray({ perturbedRenderedScenes[index].first.color }));
    pertPlusFramebuffer->setDepthAttachment(perturbedRenderedScenes[index].first.depth);
}

void GradientEstimation::initPerViewData(int num)
{
    if (perturbedRenderedScenes.size() != num)
    {
        while (perturbedRenderedScenes.size() > num)
        {
            perturbedRenderedScenes.pop_back();
        }
        while (perturbedRenderedScenes.size() < num)
        {
            perturbedRenderedScenes.push_back({ RenderedScene(screenResolution.x, screenResolution.y),
                                                RenderedScene(screenResolution.x, screenResolution.y) });
        }
    }

    while (pixelParamsPerView.size() < num)
    {
        pixelParamsPerView.push_back(
            renderer::make_ssbo<visual::PixelParamData>(screenResolution.x * screenResolution.y, GL_DYNAMIC_COPY));
    }
    while (pixelParamsPerView.size() > num)
    {
        pixelParamsPerView.pop_back();
    }
}
