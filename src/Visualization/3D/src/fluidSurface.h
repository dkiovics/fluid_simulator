#pragma once
#include "3D/src/fluidRenderer.h"
#include "engine/shaderProgram.h"
#include "geometries/basicGeometries.h"
#include "engine/framebuffer.h"
#include "compute/computeProgram.h"

namespace visual
{

class FluidSurface : public visual::FluidRenderer
{
private:
	std::shared_ptr<renderer::GpuProgram> particleSpritesDepthShader;
	std::shared_ptr<renderer::GpuProgram> gaussianBlurShaderX;
	std::shared_ptr<renderer::GpuProgram> gaussianBlurShaderY;
	std::shared_ptr<renderer::GpuProgram> bilateralFilterShaderX;
	std::shared_ptr<renderer::GpuProgram> bilateralFilterShaderY;
	std::shared_ptr<renderer::GpuProgram> shadedDepthShader;
	std::shared_ptr<renderer::GpuProgram> fluidThicknessShader;
	std::shared_ptr<renderer::GpuProgram> fluidThicknessBlurShader;
	std::shared_ptr<renderer::GpuProgram> normalAndDepthShader;

	std::unique_ptr<renderer::InstancedGeometry> instancedParticles;
	std::unique_ptr<renderer::Square> fullScreenQuad;

	std::unique_ptr<renderer::Framebuffer> depthFramebuffer;
	std::unique_ptr<renderer::Framebuffer> depthBlurTmpFramebuffer;
	std::unique_ptr<renderer::Framebuffer> fluidThicknessFramebuffer;
	std::unique_ptr<renderer::Framebuffer> fluidThicknessBlurTmpFramebuffer;
	std::unique_ptr<renderer::Framebuffer> normalAndDepthFramebuffer;

	std::shared_ptr<renderer::ComputeProgram> paramCopyProgram;

	static constexpr int PARAM_NUM_X = 10;
	struct PixelParamDataX
	{
		int paramNum;
		int paramIndexes[PARAM_NUM_X];
	};
	std::shared_ptr<renderer::StorageBuffer<PixelParamDataX>> depthParamBufferBlurX;

	float noiseOffset = 0.0f;

public:
	FluidSurface(glm::ivec2 resolution, std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights);

	void renderFluid(float dt, renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data, 
		renderer::fb_ptr fb, renderer::ssbo_ptr<PixelParamData> pixelParamData = nullptr) override;

	void setScreenResolution(glm::ivec2 resolution) override;
};

} // namespace visual
