#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "engineUtils/lights.hpp"
#include "engineUtils/camera3D.hpp"
#include "engine/framebuffer.h"
#include "manager/simulationManager.h"
#include "gfxInterface.hpp"

namespace visual
{

class FluidRenderer
{
protected:
	std::shared_ptr<renderer::Camera3D> camera;
	std::shared_ptr<renderer::Lights> lights;

public:
	FluidRenderer(std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights)
		: camera(camera), lights(lights) {}

	virtual void renderFluid(float dt, renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data, 
		renderer::fb_ptr fb, renderer::ssbo_ptr<PixelParamData> pixelParamData = nullptr) = 0;

	virtual void setScreenResolution(glm::ivec2 resolution) { }
	
	virtual ~FluidRenderer() = default;
};

} // namespace visual

