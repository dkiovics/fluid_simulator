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
	FluidSurfaceGfx(std::shared_ptr<renderer::RenderEngine> engine, std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager,
		std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights, unsigned int maxParticleNum);

	void render(std::shared_ptr<renderer::Framebuffer> renderTargetFramebuffer);

	void setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager);

	ParamColor particleColor = ParamColor("Particle color", glm::vec3(0.0, 0.4, 0.95));
	ParamBool bilateralFilterEnabled = ParamBool("Bilateral filter", false);
	ParamFloat smoothingSize = ParamFloat("Gaussian smoothing", 2.4f, 0.01f, 6.0f);
	ParamFloat blurScale = ParamFloat("Blur scale", 0.12f, 0.01f, 0.5f);
	ParamFloat blurDepthFalloff = ParamFloat("Blur depth falloff", 1000.0f, 100.0f, 10000.0f);
	ParamBool sprayEnabled = ParamBool("Spray", false);
	ParamFloat sprayDensityThreashold = ParamFloat("Spray density threashold", 1.2f, 0.0f, 10.0f);

private:
	void updateParticleColorsAndPositions();

	std::shared_ptr<renderer::RenderEngine> engine;
	std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager;
	std::shared_ptr<renderer::Camera3D> camera;
	std::shared_ptr<renderer::Lights> lights;
	std::shared_ptr<renderer::ShaderProgram> particleSpritesDepthShader;
	std::shared_ptr<renderer::ShaderProgram> normalShader;
	std::shared_ptr<renderer::ShaderProgram> gaussianBlurShader;
	std::shared_ptr<renderer::ShaderProgram> bilateralFilterShader;
	std::shared_ptr<renderer::ShaderProgram> shadedDepthShader;
	std::shared_ptr<renderer::ShaderProgram> particleSpritesShadedShader;
	std::unique_ptr<renderer::Object3D<renderer::BasicPosGeometryArray>> surfaceSquareArrayObject;
	std::unique_ptr<renderer::Object3D<renderer::BasicPosGeometryArray>> spraySquareArrayObject;
	std::unique_ptr<renderer::Square> square;
	std::unique_ptr<renderer::Object3D<renderer::Square>> shadedSquareObject;

	std::unique_ptr<renderer::Framebuffer> depthFramebuffer;
	std::shared_ptr<renderer::RenderTargetTexture> depthTexture;
	std::unique_ptr<renderer::Framebuffer> depthBlurTmpFramebuffer;
	std::shared_ptr<renderer::RenderTargetTexture> depthBlurTmpTexture;

	glm::vec3 prevColor = glm::vec3(0.0f);
	bool particleSpeedColorWasEnabled = false;
	unsigned int prevParticleNum = 0;
};
