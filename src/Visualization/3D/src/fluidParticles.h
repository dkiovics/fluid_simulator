#pragma once
#include "3D/src/fluidRenderer.h"
#include "engine/shaderProgram.h"
#include "engineUtils/object.h"
#include "geometries/geometry.h"
#include "engine/framebuffer.h"
#include "compute/computeProgram.h"
#include "compute/prefixSum.h"

namespace visual
{

class FluidParticles : public visual::FluidRenderer
{
private:
	std::unique_ptr<renderer::Object3D<renderer::InstancedGeometry>> particleSprites;
	std::shared_ptr<renderer::GpuProgram> particleSpritesShader;
	std::shared_ptr<renderer::RenderTargetTexture> paramTmpTexture;
	std::unique_ptr<renderer::ComputeProgram> particleParamsProgram;
	std::unique_ptr<renderer::PrefixSum> prefixSum;

public:
	FluidParticles(glm::ivec2 screenSize, std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights);

	void renderFluid(float dt, renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
		renderer::fb_ptr fb, pixel_params_ptr pixelParamBuffers = nullptr) override;

	void setScreenResolution(glm::ivec2 resolution) override;
};

} // namespace visual
