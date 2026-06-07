#pragma once
#include "3D/visuals3D.h"
#include "compute/computeProgram.h"
#include "engine/framebuffer.h"
#include "manager/simulationManager.h"

namespace diffrender
{

class DensityControl;
class GradientEstimation;
class AdamOptimizer;
class MiscDataUtils;
class GradientSmoothing;
class GradientVisualization;

struct ReferenceData
{
    renderer::render_target_ptr colorTexture;
    renderer::render_target_ptr depthTexture;
    renderer::Camera3D::CameraData cameraData;

    bool valid = false;
};

class StateReconstructionController
{
private:
    renderer::fb_ptr canvas;

    std::shared_ptr<genericfsim::manager::SimulationManager> simManager;
    std::shared_ptr<visual::Visuals3D> visuals3D;
    std::shared_ptr<DensityControl> densityControl;
    std::shared_ptr<GradientEstimation> gradientEstimation;
    std::shared_ptr<MiscDataUtils> miscDataUtils;
    std::shared_ptr<AdamOptimizer> adamOptimizer;
    std::unique_ptr<GradientSmoothing> gradientSmoothing;
    std::unique_ptr<GradientVisualization> gradientVisualization;

    // Copies the reconstructed state into the simulator, zeroing velocities.
    renderer::compute_ptr copyToSimulatorProgram;

    // Sums per-pixel |refColor - curColor| and dot(diff,diff) into a packed
    // float SSBO (2 floats per view, all views in one buffer). Dispatches for
    // each view in cycle N go in; readback happens in cycle N+1 — that way the
    // map(GL_MAP_READ_BIT) finds the work already done and never stalls.
    renderer::compute_ptr errorValueProgram;
    renderer::ssbo_ptr<float> errorValueSSBO;
    // Per-slot validity captured at dispatch time, so the next-cycle reader
    // knows which entries are real numbers vs. uninitialised (NaN) outputs.
    std::vector<bool> errorValuePendingViewValid;
    bool errorValueHasPendingDispatch = false;
    glm::ivec2 errorValuePendingResolution = glm::ivec2(0, 0);
    // Off-screen target used by logErrorValueIfEnabled to render each view's
    // camera without disturbing the canvas. Sized to gradient resolution to
    // keep the per-view render cost down (same fidelity as the optimization).
    renderer::fb_ptr errorRenderFramebuffer;

    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> particleData;

    renderer::fb_ptr referenceFramebuffer;

    std::vector<ReferenceData> referenceData;

    int errorValueLogPrescaler = 0;

    // Held-out reference view used only for evaluation (not fed to gradient
    // estimation). Same shape as the optimization views — color + depth + camera.
    ReferenceData evaluationData;

    void initStateReconstruction();
    void handleSpecChanges(int viewCount);
    void handleCanvasSizeChanged();
    void logErrorValueIfEnabled();

    // Maps `state.view_selection` (extended to N+1 options: N optimization views
    // followed by the evaluation view) to the right ReferenceData. Returns
    // nullptr for an out-of-range index.
    ReferenceData* getSelectedReferenceView(int viewIdx);

    glm::ivec2 getGradientResolution() const;
    int lastResolutionDivider = 0;
    int prevSelectedVeiwIdx = -1;

    enum class OperationState
    {
        IDLE,
        START_GRAD_ESTIMATION,
        GRAD_ESTIMATION,
        PARAM_OPTIMIZATION
    };

    OperationState operationState = OperationState::IDLE;

public:
    StateReconstructionController(
        renderer::fb_ptr canvas, std::shared_ptr<genericfsim::manager::SimulationManager> simManager,
                                  std::shared_ptr<visual::Visuals3D> visuals3D);

    void processAndRender();

    ~StateReconstructionController();
};

} // namespace diffrender
