#include "stateReconstructionController.h"
#include <spdlog/spdlog.h>
#include "adam/adam.h"
#include "controlRegistry.h"
#include "dataUtils/dataUtils.h"
#include "engine/glUtils.hpp"
#include "engineUtils/genericGpuPrograms.h"
#include "gradientEstimation/src/gradientEstPos.h"

using namespace diffrender;

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

    particleData =
        renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(simManager->getParticleNum(), GL_DYNAMIC_COPY);

    glm::ivec2 resolution = canvas->getSize();
    referenceData.push_back(ReferenceData());
    referenceData[0].colorTexture = renderer::make_render_target(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST,
                                                                 GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    referenceData[0].depthTexture = renderer::make_render_target(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST,
                                                                 GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
    referenceFramebuffer = renderer::make_fb({ referenceData[0].colorTexture }, referenceData[0].depthTexture, false);
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
    referenceFramebuffer->setSize(canvas->getSize());
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

void StateReconstructionController::handleSpecChanges(int viewCount)
{
    auto& registry = controls::ControlRegistry::getInstance();
    const float particleRadius = (float) registry["sim.particle_radius"];

    const glm::vec3 cellD = simManager->getCellD();
    const glm::vec3 lowerBound = cellD + particleRadius;
    const glm::vec3 upperBound = glm::vec3(simManager->getDimensions()) - cellD - particleRadius;
    gradientEstimation->setFluidBoxBounds(lowerBound, upperBound);
    adamOptimizer->setFluidBoxBounds(lowerBound, upperBound);

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
            referenceData[i].colorTexture = renderer::make_render_target(
                canvas->getSize().x, canvas->getSize().y, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
            valid = false;
        }
        if (!referenceData[i].depthTexture)
        {
            referenceData[i].depthTexture =
                renderer::make_render_target(canvas->getSize().x, canvas->getSize().y, GL_NEAREST, GL_NEAREST,
                                             GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
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

    handleCanvasSizeChanged();
    handleSpecChanges(viewCount);

    if (useViewCamera && referenceData[currentCameraPosIdx].valid)
    {
        visuals3D->getCamera()->setCameraData(referenceData[currentCameraPosIdx].cameraData);
    }

    if (resetStateReconstruction)
    {
        operationState = OperationState::IDLE;
        registry["state.run_state_reconstruction"] = false;
    }

    // If we only want to view the simulator, we can skip the state reconstruction and just render the simulator
    if (windowMode == 0)
    {
        if (updateReferenceImage)
        {
            referenceFramebuffer->setColorAttachments({ referenceData[currentCameraPosIdx].colorTexture });
            referenceFramebuffer->setDepthAttachment(referenceData[currentCameraPosIdx].depthTexture);
            visuals3D->render(canvas->getSize(), simManager->getParticleData(), referenceFramebuffer);
            referenceData[currentCameraPosIdx].cameraData = visuals3D->getCamera()->getCameraData();
            referenceData[currentCameraPosIdx].valid = true;
        }

        visuals3D->render(canvas->getSize(), simManager->getParticleData(), canvas);
        registry["state.run_state_reconstruction"] = false;
        return;
    }

    if (windowMode == 1)
    {
        if (referenceData[currentCameraPosIdx].valid)
        {
            renderer::GenericGpuPrograms::instance().copyTextureToFramebuffer(
                referenceData[currentCameraPosIdx].colorTexture,
                referenceData[currentCameraPosIdx].colorTexture->getSize(), canvas);
        }
        else
        {
            spdlog::warn("StateReconstructionController::processAndRender: Invalid reference data");
            registry["state.window_mode"] = 0;
        }
        registry["state.run_state_reconstruction"] = false;
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
        if (stateReconstructionEnabled)
        {
            switch (operationState)
            {
                case OperationState::IDLE:
                    initStateReconstruction();
                    operationState = OperationState::START_GRAD_ESTIMATION;
                    break;
                case OperationState::START_GRAD_ESTIMATION:
                    gradientEstimation->startGradientEstimation(particleData, referenceData, canvas,
                                                                currentCameraPosIdx);
                    operationState = OperationState::GRAD_ESTIMATION;
                    break;
                case OperationState::GRAD_ESTIMATION:
                    if (gradientEstimation->executeGradientEstimationStep())
                    {
                        operationState = OperationState::PARAM_OPTIMIZATION;
                    }
                    break;
                case OperationState::PARAM_OPTIMIZATION:
                    adamOptimizer->optimize(particleData, gradientEstimation->getGradientEstimationResult());
                    if (densityControlEnabled)
                    {
                        if (densityControl->updateAvgMovement(adamOptimizer->getLastParticleMovementAbs()))
                        {
                            densityControl->updatePositions(particleData);
                        }
                    }
                    operationState = OperationState::START_GRAD_ESTIMATION;
                    break;
            }
        }
        else
        {
            visuals3D->render(canvas->getSize(), particleData, canvas);
        }
    }
}
