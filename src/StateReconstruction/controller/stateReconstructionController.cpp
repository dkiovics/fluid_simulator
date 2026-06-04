#include "stateReconstructionController.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <limits>
#include "adam/adam.h"
#include "controlRegistry.h"
#include "dataUtils/dataUtils.h"
#include "engine/glUtils.hpp"
#include "engineUtils/genericGpuPrograms.h"
#include "gradientEstimation/src/gradientEstPos.h"
#include "gradientSmoothing/gradientSmoothing.h"
#include "gradientVisualization/gradientVisualization.h"

using namespace diffrender;

glm::ivec2 StateReconstructionController::getGradientResolution() const
{
    auto& registry = controls::ControlRegistry::getInstance();
    const int divider = std::max(1, (int) registry["state.resolution_divider"]);
    return glm::max(canvas->getSize() / divider, glm::ivec2(1, 1));
}

StateReconstructionController::StateReconstructionController(
    renderer::fb_ptr canvas,
    std::shared_ptr<genericfsim::manager::SimulationManager> simManager,
    std::shared_ptr<visual::Visuals3D> visuals3D)
    : simManager(simManager), visuals3D(visuals3D), canvas(canvas)
{
    densityControl = std::make_shared<DensityControl>(simManager);
    gradientEstimation = std::make_shared<GradientEstPos>(canvas->getSize(), visuals3D);
    miscDataUtils = std::make_shared<MiscDataUtils>();
    adamOptimizer = std::make_shared<AdamOptimizer>();
    gradientVisualization = std::make_unique<GradientVisualization>(visuals3D->getCamera(), visuals3D->getLights());

    copyToSimulatorProgram = renderer::make_compute("shaders/stateReconstruction/copyParticlesZeroVelocity.comp");

    errorValueProgram = renderer::make_compute("shaders/stateReconstruction/errorValue.comp");
    errorValueSSBO = renderer::make_ssbo<float>(1, GL_DYNAMIC_READ);

    particleData =
        renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(simManager->getParticleNum(), GL_DYNAMIC_COPY);

    glm::ivec2 resolution = canvas->getSize();
    {
        auto color = renderer::make_render_target(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST,
                                                  GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
        auto depth = renderer::make_render_target(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST,
                                                  GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
        errorRenderFramebuffer = renderer::make_fb({ color }, depth, false);
    }
    referenceData.push_back(ReferenceData());
    referenceData[0].colorTexture = renderer::make_render_target(resolution.x, resolution.y, GL_LINEAR_MIPMAP_LINEAR,
                                                                 GL_LINEAR, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    referenceData[0].depthTexture =
        renderer::make_render_target(resolution.x, resolution.y, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
                                     GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
    referenceFramebuffer = renderer::make_fb({ referenceData[0].colorTexture }, referenceData[0].depthTexture, false);

    // Held-out evaluation view — same texture shapes as the optimization views,
    // but lives outside `referenceData` so gradient estimation never sees it.
    evaluationData.colorTexture = renderer::make_render_target(resolution.x, resolution.y, GL_LINEAR_MIPMAP_LINEAR,
                                                               GL_LINEAR, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    evaluationData.depthTexture =
        renderer::make_render_target(resolution.x, resolution.y, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR,
                                     GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
    evaluationData.valid = false;

    // Sync gradient estimation to initial resolution
    gradientEstimation->handleResolutionChanged(getGradientResolution());
}

void StateReconstructionController::handleCanvasSizeChanged()
{
    if (canvas->getSize() == referenceFramebuffer->getSize())
        return;
    for (auto& ref : referenceData)
    {
        ref.colorTexture->resizeTexture(canvas->getSize().x, canvas->getSize().y);
        ref.depthTexture->resizeTexture(canvas->getSize().x, canvas->getSize().y);
        ref.valid = false;
    }
    if (evaluationData.colorTexture)
        evaluationData.colorTexture->resizeTexture(canvas->getSize().x, canvas->getSize().y);
    if (evaluationData.depthTexture)
        evaluationData.depthTexture->resizeTexture(canvas->getSize().x, canvas->getSize().y);
    evaluationData.valid = false;
    referenceFramebuffer->setSize(canvas->getSize());
    if (errorRenderFramebuffer)
        errorRenderFramebuffer->setSize(canvas->getSize());
    gradientEstimation->handleResolutionChanged(getGradientResolution());
    lastResolutionDivider = std::max(1, (int) controls::ControlRegistry::getInstance()["state.resolution_divider"]);
    operationState = OperationState::IDLE;
}

void StateReconstructionController::initStateReconstruction()
{
    auto& registry = controls::ControlRegistry::getInstance();

    particleData->setSize(simManager->getParticleNum());
    simManager->getParticleData()->copyTo(*particleData);

    densityControl->init(simManager->getParticleNum());
    adamOptimizer->init(simManager->getParticleNum());
}

StateReconstructionController::~StateReconstructionController() {}

ReferenceData* StateReconstructionController::getSelectedReferenceView(int viewIdx)
{
    if (viewIdx >= 0 && viewIdx < (int) referenceData.size())
        return &referenceData[viewIdx];
    if (viewIdx == (int) referenceData.size())
        return &evaluationData;
    return nullptr;
}

void StateReconstructionController::logErrorValueIfEnabled()
{
    auto& registry = controls::ControlRegistry::getInstance();
    if (!(bool) registry["state.log_error_value"])
        return;
    if (!errorRenderFramebuffer)
        return;

    errorValueLogPrescaler++;
    if (errorValueLogPrescaler < 5)
    {
        return;
    }
    errorValueLogPrescaler = 0;

    // Computes the per-pixel L2-error sum between errorRenderFramebuffer's
    // color attachment (filled by the caller via visuals3D->render) and
    // `referenceTexture`. Same metric as stochGradient_color.comp.
    auto computeError = [&](const std::shared_ptr<renderer::RenderTargetTexture>& referenceTexture) -> float
    {
        errorValueSSBO->fillWithZeros();
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

        (*errorValueProgram)["referenceImage"] = *referenceTexture;
        (*errorValueProgram)["currentImage"] = *errorRenderFramebuffer->getColorAttachments()[0];
        (*errorValueProgram)["screenSize"] = errorRenderFramebuffer->getSize();

        errorValueSSBO->bindBuffer(0);
        const glm::ivec2 sz = errorRenderFramebuffer->getSize();
        errorValueProgram->dispatchCompute((sz.x + 15) / 16, (sz.y + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        errorValueSSBO->mapBuffer(0, 1, GL_MAP_READ_BIT);
        const float v = (*errorValueSSBO)[0];
        errorValueSSBO->unmapBuffer();
        return v;
    };

    // Render `particleData` from `view`'s camera into errorRenderFramebuffer,
    // then compute error against the view's stored reference image. Returns
    // NaN if the view has no captured reference yet.
    auto evaluateView = [&](const ReferenceData& view) -> float
    {
        if (!view.valid || !view.colorTexture)
            return std::numeric_limits<float>::quiet_NaN();
        visuals3D->getCamera()->setCameraData(view.cameraData);
        visuals3D->render(errorRenderFramebuffer->getSize(), particleData, errorRenderFramebuffer);
        return computeError(view.colorTexture);
    };

    // Save the camera so downstream consumers of canvas (e.g. arrow overlay)
    // see the original view-selection camera state, not whatever the last
    // evaluated view set it to.
    const auto savedCamera = visuals3D->getCamera()->getCameraData();

    std::ostringstream oss;
    for (size_t i = 0; i < referenceData.size(); i++)
    {
        if (i > 0)
            oss << ';';
        oss << evaluateView(referenceData[i]);
    }
    oss << ';' << evaluateView(evaluationData);

    visuals3D->getCamera()->setCameraData(savedCamera);

    std::cout << oss.str() << std::endl;
}

void StateReconstructionController::handleSpecChanges(int viewCount)
{
    auto& registry = controls::ControlRegistry::getInstance();
    const float particleRadius = (float) registry["sim.particle_radius"];

    const glm::vec3 cellD = simManager->getCellD();
    const glm::vec3 lowerBound = cellD + particleRadius;
    const glm::vec3 upperBound = glm::vec3(simManager->getDimensions()) - cellD - particleRadius;
    gradientEstimation->setFluidBoxBounds(lowerBound, upperBound);
    adamOptimizer->setFluidBoxBounds(lowerBound, upperBound);

    // Check if the resolution divider changed
    const int currentDivider = std::max(1, (int) registry["state.resolution_divider"]);
    if (currentDivider != lastResolutionDivider)
    {
        lastResolutionDivider = currentDivider;
        gradientEstimation->handleResolutionChanged(getGradientResolution());
        if (operationState != OperationState::IDLE)
            operationState = OperationState::START_GRAD_ESTIMATION;
    }

    if (referenceData.size() != viewCount)
    {
        referenceData.resize(viewCount);
        operationState = OperationState::IDLE;
    }
    for (int i = 0; i < viewCount; i++)
    {
        bool valid = true;
        if (!referenceData[i].colorTexture)
        {
            referenceData[i].colorTexture =
                renderer::make_render_target(canvas->getSize().x, canvas->getSize().y, GL_LINEAR_MIPMAP_LINEAR,
                                             GL_LINEAR, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
            valid = false;
        }
        if (!referenceData[i].depthTexture)
        {
            referenceData[i].depthTexture =
                renderer::make_render_target(canvas->getSize().x, canvas->getSize().y, GL_LINEAR_MIPMAP_LINEAR,
                                             GL_LINEAR, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
            valid = false;
        }
        referenceData[i].valid = referenceData[i].valid && valid;
        if (!valid)
            operationState = OperationState::IDLE;
    }

    if (particleData->getSize() != simManager->getParticleNum())
    {
        operationState = OperationState::IDLE;
    }
}

// RadioButton windowMode{ "state.window_mode", { "View simulator", "View reference data", "View reconstructed state" }
// }; CheckBox runStateReconstruction{ "state.run_state_reconstruction", "Run state reconstruction" }; Button
// resetStateReconstruction{ "state.reset_state_reconstruction", "Reset state reconstruction" }; CheckBox
// densityControlEnabled{ "state.density_control_enabled", "Density control" }; CheckBox capPositionsToBox{
// "state.cap_positions_to_box", "Cap positions to box", true }; CheckBox useDepthImage{ "state.use_depth_image", "Use
// depth image", false }; Button updateReferenceImage{ "state.update_reference_image", "Update reference image" };
// SliderInt referenceViewCount{ "state.reference_view_count", "Reference view count", 1, 8, 1 };
// RadioButton viewSelection{ "state.view_selection", { "View 1" } };
// Button useViewCamera{ "state.use_view_camera", "Use view camera" };

void StateReconstructionController::processAndRender()
{
    auto& registry = controls::ControlRegistry::getInstance();
    const bool stateReconstructionEnabled = (bool) registry["state.run_state_reconstruction"];
    const bool resetStateReconstruction = (bool) registry["state.reset_state_reconstruction"];
    if (resetStateReconstruction)
        registry["state.reset_state_reconstruction"] = false;
    const int windowMode = (int) registry["state.window_mode"];
    const bool densityControlEnabled = (bool) registry["state.density_control_enabled"];
    const bool updateReferenceImage = (bool) registry["state.update_reference_image"];
    if (updateReferenceImage)
        registry["state.update_reference_image"] = false;
    const int viewCount = (int) registry["state.reference_view_count"];
    const int currentCameraPosIdx = (int) registry["state.view_selection"];
    const bool useViewCamera = (bool) registry["state.use_view_camera"];
    if (useViewCamera)
        registry["state.use_view_camera"] = false;
    const bool updateSimulator = (bool) registry["state.update_simulator"];
    if (updateSimulator)
        registry["state.update_simulator"] = false;

    handleCanvasSizeChanged();
    handleSpecChanges(viewCount);

    if (viewCount < currentCameraPosIdx)
    {
        registry["state.view_selection"] = viewCount;
    }

    // Copy the reconstructed state back into the actual simulator, zeroing
    // velocities so the simulator restarts from rest. Requires matching SSBO
    // sizes; if the user changed the particle count after the last
    // reconstruction step, we skip rather than risking a corrupt copy.
    if (updateSimulator)
    {
        auto simParticleData = simManager->getParticleData();
        if (simParticleData && particleData->getSize() == simParticleData->getSize())
        {
            particleData->bindBuffer(0);
            simParticleData->bindBuffer(1);
            copyToSimulatorProgram->dispatchCompute(particleData->getSize() / 64 + 1, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            simManager->setParticleData(particleData);
        }
        else
        {
            spdlog::warn(
                "StateReconstructionController: 'update simulator state' skipped — "
                "particle count mismatch (reconstructed={}, simulator={})",
                particleData ? particleData->getSize() : 0, simParticleData ? simParticleData->getSize() : 0);
        }
    }

    if (useViewCamera)
    {
        if (auto* view = getSelectedReferenceView(currentCameraPosIdx); view && view->valid)
        {
            visuals3D->getCamera()->setCameraData(view->cameraData);
        }
    }

    if (resetStateReconstruction)
    {
        operationState = OperationState::IDLE;
        registry["state.run_state_reconstruction"] = false;
        spdlog::info("State reconstruction reset");
        errorValueLogPrescaler = 0;
    }

    // If we only want to view the simulator, we can skip the state reconstruction and just render the simulator
    if (windowMode == 0)
    {
        if (updateReferenceImage)
        {
            if (auto* view = getSelectedReferenceView(currentCameraPosIdx))
            {
                referenceFramebuffer->setColorAttachments({ view->colorTexture });
                referenceFramebuffer->setDepthAttachment(view->depthTexture);
                visuals3D->render(canvas->getSize(), simManager->getParticleData(), referenceFramebuffer);
                view->cameraData = visuals3D->getCamera()->getCameraData();
                view->valid = true;
                view->colorTexture->generateMipmaps();
            }
        }

        visuals3D->render(canvas->getSize(), simManager->getParticleData(), canvas);
        registry["state.run_state_reconstruction"] = false;
        return;
    }

    if (windowMode == 1)
    {
        auto* view = getSelectedReferenceView(currentCameraPosIdx);
        if (view && view->valid && view->colorTexture)
        {
            renderer::GenericGpuPrograms::instance().copyTextureToFramebuffer(view->colorTexture,
                                                                              view->colorTexture->getSize(), canvas);
        }
        else
        {
            spdlog::warn("StateReconstructionController::processAndRender: Invalid reference data");
            registry["state.window_mode"] = 0;
        }
        return;
    }

    if (windowMode == 2)
    {
        for (int i = 0; i < (int) referenceData.size(); i++)
        {
            if (!referenceData[i].valid)
            {
                spdlog::warn("StateReconstructionController::processAndRender: Invalid reference data for view {}", i);
                registry["state.window_mode"] = 0;
                registry["state.run_state_reconstruction"] = false;
                return;
            }
        }
        const bool enableGradientSmoothing = (bool) registry["state.enable_gradient_smoothing"];
        const float gradientSmoothingSphereR = (float) registry["state.gradient_smoothing_sphere_r"];
        const bool gradientVisualizationEnabled = (bool) registry["state.gradient_visualization"];
        const float arrowDensityThreshold = (float) registry["state.arrow_density_threshold"];

        // Lazily (re)create the smoother whenever the sim box or the smoothing
        // radius changes — both are inputs to the spatial-hash cell grid size.
        const glm::vec3 simBox = simManager->getDimensions();
        if (enableGradientSmoothing && (!gradientSmoothing || gradientSmoothing->particleBox != simBox ||
                                        gradientSmoothing->smoothingSphereR != gradientSmoothingSphereR))
        {
            gradientSmoothing = std::make_unique<GradientSmoothing>(gradientSmoothingSphereR, simBox);
        }

        if (stateReconstructionEnabled)
        {
            switch (operationState)
            {
                case OperationState::IDLE:
                    initStateReconstruction();
                    operationState = OperationState::START_GRAD_ESTIMATION;
                    break;
                case OperationState::START_GRAD_ESTIMATION:
                    gradientEstimation->startGradientEstimation(particleData, referenceData);
                    operationState = OperationState::GRAD_ESTIMATION;
                    break;
                case OperationState::GRAD_ESTIMATION:
                    if (gradientEstimation->executeGradientEstimationStep())
                    {
                        operationState = OperationState::PARAM_OPTIMIZATION;
                    }
                    break;
                case OperationState::PARAM_OPTIMIZATION:
                {
                    auto gradient = gradientEstimation->getGradientEstimationResult();
                    // Smoothing mutates the gradient in-place so Adam sees the smoothed
                    // values; visualization snapshots after smoothing so the rendered
                    // arrows reflect what was actually applied to the parameters.
                    if (enableGradientSmoothing && gradient)
                    {
                        gradientSmoothing->smoothGradient(particleData, gradient);
                    }
                    if (gradient)
                    {
                        gradientVisualization->snapshotGradient(gradient);
                    }
                    adamOptimizer->optimize(particleData, gradient);
                    if (densityControlEnabled)
                    {
                        if (densityControl->updateAvgMovement(adamOptimizer->getLastParticleMovementAbs()))
                        {
                            densityControl->updatePositions(particleData);
                        }
                    }
                    if (auto* view = getSelectedReferenceView(currentCameraPosIdx); view && view->valid)
                    {
                        visuals3D->getCamera()->setCameraData(view->cameraData);
                    }
                    visuals3D->render(canvas->getSize(), particleData, canvas);
                    // Log error BEFORE arrow overlay so the comparison is against the
                    // clean rendered scene rather than scene + arrow pixels.
                    logErrorValueIfEnabled();
                    if (gradientVisualizationEnabled)
                    {
                        simManager->computeParticleDensities(particleData);
                        gradientVisualization->render(particleData, canvas, arrowDensityThreshold);
                    }
                    operationState = OperationState::START_GRAD_ESTIMATION;
                    break;
                }
            }
        }
        else
        {
            if (prevSelectedVeiwIdx != currentCameraPosIdx)
            {
                prevSelectedVeiwIdx = currentCameraPosIdx;
                if (auto* view = getSelectedReferenceView(currentCameraPosIdx); view && view->valid)
                {
                    visuals3D->getCamera()->setCameraData(view->cameraData);
                }
            }
            visuals3D->render(canvas->getSize(), particleData, canvas);
            if (gradientVisualizationEnabled)
            {
                gradientVisualization->render(particleData, canvas, arrowDensityThreshold);
            }
        }
    }
}
