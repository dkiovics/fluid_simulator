#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <engineUtils/lights.hpp>
#include <engineUtils/camera3D.hpp>
#include <geometries/basicGeometries.h>
#include <engine/renderEngine.h>
#include <engine/shaderProgram.h>
#include <engine/framebuffer.h>
#include <engine/texture.h>
#include <engineUtils/object.h>
#include "manager/simulationManager.h"
#include "../../ui/param.hpp"

namespace gfx3D
{

class FluidSurfaceGfx : public ParamLineCollection
{
public:
	FluidSurfaceGfx(std::shared_ptr<renderer::RenderEngine> engine, std::shared_ptr<renderer::Framebuffer> renderTargetFramebuffer, std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager,
		std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights, unsigned int maxParticleNum);

	void render();

	void setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager);

	ParamColor particleColor = ParamColor("Fluid color", glm::vec3(0.0, 0.4, 0.95));
	ParamBool bilateralFilterEnabled = ParamBool("Bilateral filter", false);
	ParamFloat smoothingSize = ParamFloat("Gaussian smoothing", 2.4f, 0.01f, 6.0f);
	ParamFloat blurScale = ParamFloat("Blur scale", 0.083f, 0.01f, 0.4f);
	ParamFloat blurDepthFalloff = ParamFloat("Blur depth falloff", 1100.0f, 100.0f, 10000.0f);
	ParamBool sprayEnabled = ParamBool("Spray", true);
	ParamFloat sprayDensityThreashold = ParamFloat("Spray density threashold", 1.2f, 0.0f, 10.0f);

	ParamBool fluidTransparencyEnabled = ParamBool("Transparent fluid", true);
	ParamFloat fluidTransparencyBlurSize = ParamFloat("Fluid thickness blur size", 2.4f, 0.01f, 6.0f);
	ParamFloat fluidTransparency = ParamFloat("Fluid transparency", 1.334f, 0.0f, 3.0f);

	ParamBool fluidSurfaceNoiseEnabled = ParamBool("Fluid surface noise", true);
	ParamFloat fluidSurfaceNoiseScale = ParamFloat("Noise scale", 0.956f, 0.01f, 5.0f);
	ParamFloat fluidSurfaceNoiseStrength = ParamFloat("Noise strength", 0.123f, 0.0f, 1.0f);
	ParamFloat fluidSurfaceNoiseSpeed = ParamFloat("Noise speed", 0.317, 0.0f, 1.0f);

private:
	void updateParticleData();

	std::shared_ptr<renderer::RenderEngine> engine;
	std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager;
	std::shared_ptr<renderer::Camera3D> camera;
	std::shared_ptr<renderer::Lights> lights;

	std::shared_ptr<renderer::GpuProgram> particleSpritesDepthShader;
	std::shared_ptr<renderer::GpuProgram> gaussianBlurShader;
	std::shared_ptr<renderer::GpuProgram> bilateralFilterShader;
	std::shared_ptr<renderer::GpuProgram> shadedDepthShader;
	std::shared_ptr<renderer::GpuProgram> fluidThicknessShader;
	std::shared_ptr<renderer::GpuProgram> fluidThicknessBlurShader;
	std::shared_ptr<renderer::GpuProgram> normalAndDepthShader;

	std::unique_ptr<renderer::Object3D<renderer::ParticleGeometryArray>> surfaceSquareArrayObject;
	std::unique_ptr<renderer::Object3D<renderer::ParticleGeometryArray>> spraySquareArrayObject;
	std::unique_ptr<renderer::Square> square;
	std::unique_ptr<renderer::Object3D<renderer::Square>> shadedSquareObject;

	std::shared_ptr<renderer::Framebuffer> renderTargetFramebuffer;
	std::unique_ptr<renderer::Framebuffer> depthFramebuffer;
	std::unique_ptr<renderer::Framebuffer> depthBlurTmpFramebuffer;
	std::unique_ptr<renderer::Framebuffer> fluidThicknessFramebuffer;
	std::unique_ptr<renderer::Framebuffer> fluidThicknessBlurTmpFramebuffer;
	std::unique_ptr<renderer::Framebuffer> normalAndDepthFramebuffer;

	glm::vec3 prevColor = glm::vec3(0.0f);
	bool particleSpeedColorWasEnabled = false;
	unsigned int prevParticleNum = 0;
	float noiseOffset = 0.0f;
};

}	// namespace gfx3D
