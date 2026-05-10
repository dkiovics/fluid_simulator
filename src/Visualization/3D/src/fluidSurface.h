#pragma once
#include "3D/src/fluidRenderer.h"
#include "engine/shaderProgram.h"
#include "geometries/basicGeometries.h"
#include "engine/framebuffer.h"
#include "compute/computeProgram.h"
#include "compute/prefixSum.h"

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

	// depthFramebuffer.depth holds the SPRITE depth (preserved through the whole pipeline,
	// because Y-blur output goes to finalDepthFramebuffer instead).
	// depthFramebuffer.color holds the surface paramTexture (per-pixel closest particle id).
	std::unique_ptr<renderer::Framebuffer> depthFramebuffer;
	std::unique_ptr<renderer::Framebuffer> depthBlurTmpFramebuffer; // X-blurred depth

	// finalDepthFramebuffer.depth holds the Y-blurred depth (consumed by spray, normalAndDepth, thickness).
	std::unique_ptr<renderer::Framebuffer> finalDepthFramebuffer;
	std::shared_ptr<renderer::RenderTargetTexture> finalDepthTexture;

	// sprayFramebuffer.color holds spray particle ids (-1 outside spray pixels).
	// Shares depth attachment with finalDepthFramebuffer (read-only via glDepthMask).
	std::unique_ptr<renderer::Framebuffer> sprayFramebuffer;
	std::shared_ptr<renderer::RenderTargetTexture> sprayParamTexture;

	std::unique_ptr<renderer::Framebuffer> fluidThicknessFramebuffer;
	std::unique_ptr<renderer::Framebuffer> fluidThicknessBlurTmpFramebuffer;
	std::unique_ptr<renderer::Framebuffer> normalAndDepthFramebuffer;

	std::shared_ptr<renderer::ComputeProgram> initParamsProgram;
	std::shared_ptr<renderer::ComputeProgram> xFillProgram;
	std::shared_ptr<renderer::ComputeProgram> sprayOverrideProgram;

	std::unique_ptr<renderer::PrefixSum> prefixSum;

	float noiseOffset = 0.0f;

public:
	FluidSurface(glm::ivec2 resolution, std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights);

	void renderFluid(float dt, renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
		renderer::fb_ptr fb, pixel_params_ptr pixelParamBuffers = nullptr) override;

	void setScreenResolution(glm::ivec2 resolution) override;
};

} // namespace visual
