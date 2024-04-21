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

class FluidSurfaceGfx : public ParamLineCollection
{
public:
	FluidSurfaceGfx(std::shared_ptr<renderer::RenderEngine> engine, std::shared_ptr<renderer::Framebuffer> renderTargetFramebuffer, std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager,
		std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights, unsigned int maxParticleNum);

	void render();

	void setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager);

	ParamColor particleColor = ParamColor("Particle color", glm::vec3(0.0, 0.4, 0.95));
	ParamBool bilateralFilterEnabled = ParamBool("Bilateral filter", false);
	ParamFloat smoothingSize = ParamFloat("Gaussian smoothing", 2.4f, 0.01f, 6.0f);
	ParamFloat blurScale = ParamFloat("Blur scale", 0.083f, 0.01f, 0.4f);
	ParamFloat blurDepthFalloff = ParamFloat("Blur depth falloff", 1100.0f, 100.0f, 10000.0f);
	ParamBool sprayEnabled = ParamBool("Spray", false);
	ParamFloat sprayDensityThreashold = ParamFloat("Spray density threashold", 1.2f, 0.0f, 10.0f);

	ParamBool fluidTransparencyEnabled = ParamBool("Transparent fluid", false);
	ParamFloat fluidTransparencyBlurSize = ParamFloat("Fluid thickness blur size", 2.4f, 0.01f, 6.0f);
	ParamFloat fluidTransparency = ParamFloat("Fluid transparency", 0.75f, 0.0f, 3.0f);

	ParamBool fluidSurfaceNoiseEnabled = ParamBool("Fluid surface noise", false);
	ParamFloat fluidSurfaceNoiseScale = ParamFloat("Noise scale", 1.0f, 0.01f, 5.0f);
	ParamFloat fluidSurfaceNoiseSpeedCoeff = ParamFloat("Noise speed coeff", 0.1f, 0.0f, 1.0f);
	ParamFloat fluidSurfaceNoiseStrength = ParamFloat("Noise strength", 0.1f, 0.0f, 1.0f);

private:
	void updateParticleData();

	std::shared_ptr<renderer::RenderEngine> engine;
	std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager;
	std::shared_ptr<renderer::Camera3D> camera;
	std::shared_ptr<renderer::Lights> lights;

	std::shared_ptr<renderer::ShaderProgram> particleSpritesDepthShader;
	std::shared_ptr<renderer::ShaderProgram> gaussianBlurShader;
	std::shared_ptr<renderer::ShaderProgram> bilateralFilterShader;
	std::shared_ptr<renderer::ShaderProgram> shadedDepthShader;
	std::shared_ptr<renderer::ShaderProgram> fluidThicknessAndNoiseShader;
	std::shared_ptr<renderer::ShaderProgram> fluidThicknessBlurShader;

	std::unique_ptr<renderer::Object3D<renderer::ParticleGeometryArray>> surfaceSquareArrayObject;
	std::unique_ptr<renderer::Object3D<renderer::ParticleGeometryArray>> spraySquareArrayObject;
	std::unique_ptr<renderer::Square> square;
	std::unique_ptr<renderer::Object3D<renderer::Square>> shadedSquareObject;

	std::shared_ptr<renderer::Framebuffer> renderTargetFramebuffer;
	std::unique_ptr<renderer::Framebuffer> depthFramebuffer;
	std::unique_ptr<renderer::Framebuffer> depthBlurTmpFramebuffer;
	std::unique_ptr<renderer::Framebuffer> fluidThicknessFramebuffer;
	std::unique_ptr<renderer::Framebuffer> fluidThicknessAndNoiseFramebuffer;
	std::unique_ptr<renderer::Framebuffer> fluidThicknessBlurTmpFramebuffer;

	glm::vec3 prevColor = glm::vec3(0.0f);
	bool particleSpeedColorWasEnabled = false;
	unsigned int prevParticleNum = 0;
};
