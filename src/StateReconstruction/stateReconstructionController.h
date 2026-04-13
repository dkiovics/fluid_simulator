#pragma once
#include "engine/windowManager.h"
#include "engine/framebuffer.h"
#include "manager/simulationManager.h"
#include "3D/visuals3D.h"


namespace diffrender
{

class DensityControl;
class GradientEstimation;
class AdamOptimizer;
class MiscDataUtils;

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
	renderer::WindowManager& windowManager;

	std::shared_ptr<genericfsim::manager::SimulationManager> simManager;
	std::shared_ptr<visual::Visuals3D> visuals3D;
	std::shared_ptr<DensityControl> densityControl;
	std::shared_ptr<GradientEstimation> gradientEstimation;
    std::shared_ptr<MiscDataUtils> miscDataUtils;
	std::shared_ptr<AdamOptimizer> adamOptimizer;

	renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> particleData;

	renderer::fb_ptr referenceFramebuffer;
	renderer::fb_ptr viewFramebuffer;

	glm::ivec2 resolution;
	unsigned int screenResolutionChangedCbId = 0;
	void screenResolutionChanged(int width, int height);

	std::vector<ReferenceData> referenceData;

	void initStateReconstruction();
	void handleSpecChanges(int viewCount);

	enum class OperationState
	{
        IDLE,
        START_GRAD_ESTIMATION,
        GRAD_ESTIMATION,
        PARAM_OPTIMIZATION
	};

	OperationState operationState = OperationState::IDLE;

public:
	StateReconstructionController(renderer::WindowManager& windowManager, std::shared_ptr<genericfsim::manager::SimulationManager> simManager,
		std::shared_ptr<visual::Visuals3D> visuals3D);

	void processAndRender();

	~StateReconstructionController();

};

} // namespace diffrender

