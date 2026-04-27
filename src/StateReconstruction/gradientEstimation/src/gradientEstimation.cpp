#include "gradientEstimation/gradientEstimation.h"
#include "controlRegistry.h"

using namespace diffrender;

GradientEstimation::GradientEstimation(glm::ivec2 resolution, std::shared_ptr<visual::Visuals3D> visuals3D)
    : screenResolution(1), visuals3D(visuals3D)
{
    gradientMultCompute = renderer::make_compute("shaders/stateReconstruction/gradientEstimation/gradientMult.comp");

    paramNegativeOffsetSSBO = renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(1, GL_DYNAMIC_COPY);
    paramPositiveOffsetSSBO = renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(1, GL_DYNAMIC_COPY);
    optimizedParamsSSBO = renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(1, GL_DYNAMIC_COPY);
    stochaisticGradientSSBO = renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(1, GL_DYNAMIC_COPY);

    initPerViewData(1);

    pertPlusFramebuffer =
        renderer::make_fb(renderer::Framebuffer::toArray({ perturbedRenderedScenes[0].first.color }), nullptr, false);

    pertMinusFramebuffer = std::make_shared<renderer::Framebuffer>(
        renderer::Framebuffer::toArray({ perturbedRenderedScenes[0].second.color }), nullptr, false);

    handleResolutionChanged(resolution);
}

void GradientEstimation::startGradientEstimation(
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> particleData,
    std::vector<ReferenceData> referenceData)
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
    particleData->copyTo(*optimizedParamsSSBO);
    stochaisticGradientSSBO->fillWithZeros();

    initPerViewData(referenceData.size());

    gradientSampleNum = (int) registry["state.gradient_sample_count"];
    gradientSampleCount = 0;
    gradientEstimationInProgress = true;

    for (int i = 0; i < (int) referenceData.size(); i++)
    {
        visuals3D->getCamera()->setCameraData(referenceData[i].cameraData);
        visuals3D->render(screenResolution, particleData, nullptr, pixelParamsPerView[i]);
    }
    if (registry["state.log_per_pixel_param_stats"])
    {
        logPerPixelParamStats();
    }
}

void GradientEstimation::logPerPixelParamStats() const
{
    spdlog::info("Per-pixel param stats:");
    for (const auto& params : pixelParamsPerView)
    {
        uint64_t totalUncappedParamSum = 0;
        uint64_t pixelsWithAtLeastOneUncappedParam = 0;
        uint64_t maxUncappedParamNum = 0;
        params->mapBuffer(0, -1, GL_MAP_READ_BIT);
        for (int j = 0; j < params->getSize(); j++)
        {
            const auto& paramData = (*params)[j];
            totalUncappedParamSum += paramData.uncappedParamNum;
            if (paramData.uncappedParamNum > maxUncappedParamNum)
                maxUncappedParamNum = paramData.uncappedParamNum;
            if (paramData.uncappedParamNum > 0)
                pixelsWithAtLeastOneUncappedParam++;
        }
        params->unmapBuffer();
        spdlog::info(
            "View {}:\n" 
            "   average uncapped param num = {}\n"
            "   pixels with at least one uncapped param = {}\n"
            "   max uncapped param num = {}\n"
            "   total uncapped param num = {}",
            &params - &pixelParamsPerView[0], (double) totalUncappedParamSum / params->getSize(),
            pixelsWithAtLeastOneUncappedParam, maxUncappedParamNum, totalUncappedParamSum);
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

void GradientEstimation::handleResolutionChanged(glm::ivec2 size)
{
    if (size == screenResolution)
        return;
    screenResolution = size;
    for (auto& scene : perturbedRenderedScenes)
    {
        scene.first.resize(size.x, size.y);
        scene.second.resize(size.x, size.y);
    }

    for (auto& params : pixelParamsPerView)
    {
        params->setSize(size.x * size.y);
    }

    pertPlusFramebuffer->setSize(size);
    pertMinusFramebuffer->setSize(size);
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
