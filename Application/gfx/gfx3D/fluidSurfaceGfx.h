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

class FluidSurfaceGfx
{
public:
	FluidSurfaceGfx(std::shared_ptr<renderer::RenderEngine> engine, std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager,
		std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights, unsigned int maxParticleNum);

	void render(std::shared_ptr<renderer::Framebuffer> renderTargetFramebuffer);

	void setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager);

	float maxParticleSpeed = 1.0f;
	float smoothingSize = 1.2f;
	bool particleSpeedColorEnabled = false;
	glm::vec3 particleColor = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 particleSpeedColor = glm::vec3(1.0f, 0.0f, 0.0f);

private:
	void updateParticleColorsAndPositions();

	std::shared_ptr<renderer::RenderEngine> engine;
	std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager;
	std::shared_ptr<renderer::Camera3D> camera;
	std::shared_ptr<renderer::Lights> lights;
	std::shared_ptr<renderer::ShaderProgram> pointSpritesShader;
	std::shared_ptr<renderer::ShaderProgram> normalShader;
	std::shared_ptr<renderer::ShaderProgram> gaussianBlurShader;
	std::shared_ptr<renderer::ShaderProgram> shadedDepthShader;
	std::unique_ptr<renderer::Object3D<renderer::BasicGeometryArray>> squareArrayObject;
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
