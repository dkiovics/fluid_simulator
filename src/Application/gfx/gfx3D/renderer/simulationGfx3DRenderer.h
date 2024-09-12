#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <engineUtils/lights.hpp>
#include <engineUtils/camera3D.hpp>
#include <engineUtils/object.h>
#include <geometries/basicGeometries.h>
#include <engine/renderEngine.h>
#include <engine/texture.h>
#include <engine/shaderProgram.h>
#include <engine/framebuffer.h>
#include <vector>
#include "transparentBox.hpp"
#include "../renderer3DInterface.h"
#include <param.hpp>
#include "fluidSurfaceGfx.h"

namespace gfx3D
{

class SimulationGfx3DRenderer : public Renderer3DInterface
{
public:
	SimulationGfx3DRenderer(std::shared_ptr<renderer::RenderEngine> engine, 
		std::shared_ptr<renderer::Camera3D> camera,	std::shared_ptr<renderer::Lights> lights, 
		const std::vector<std::unique_ptr<renderer::Object3D<renderer::Geometry>>>& obstacleGfxArray,
		unsigned int maxParticleNum, ConfigData3D configData);

	void setConfigData(const ConfigData3D& configData) override;

	void show(int screenWidth) override;

	void render(std::shared_ptr<renderer::Framebuffer> framebuffer,
		std::shared_ptr<renderer::RenderTargetTexture> paramTexture, const Gfx3DRenderData& data) override;

private:
	const int maxParticleNum;

	ConfigData3D configData;

	std::shared_ptr<renderer::RenderEngine> engine;
	std::shared_ptr<renderer::Camera3D> camera;
	std::shared_ptr<renderer::Lights> lights;
	const std::vector<std::unique_ptr<renderer::Object3D<renderer::Geometry>>>& obstacleGfxArray;

	std::unique_ptr<renderer::Object3D<renderer::Geometry>> planeGfx;
	std::unique_ptr<renderer::Object3D<renderer::BasicGeometryArray>> particlesGfx;

	std::unique_ptr<TransparentBox> transparentBox;

	std::shared_ptr<renderer::GpuProgram> shaderProgramTextured;
	std::shared_ptr<renderer::GpuProgram> shaderProgramNotTextured;
	std::shared_ptr<renderer::GpuProgram> shaderProgramNotTexturedArray;
	std::shared_ptr<renderer::GpuProgram> shaderProgramNotTexturedArrayWithId;

	std::shared_ptr<renderer::GpuProgram> showShaderProgram;
	std::shared_ptr<renderer::Square> showSquare;
	std::shared_ptr<renderer::RenderTargetTexture> renderTargetTexture;

	std::unique_ptr<FluidSurfaceGfx> fluidSurfaceGfx;

	enum FluidRenderMode
	{
		NONE = 0,
		PARTICLES,
		SURFACE
	};

	ParamBool showFloor = ParamBool("Show floor", true);
	ParamBool showBox = ParamBool("Show box", true);
	ParamBool showObstacles = ParamBool("Show obstacles", true);
	ParamRadio fluidRenderMode = ParamRadio("Fluid render mode", { "None", "Particles", "Surface" }, PARTICLES);

	void handleFluidRenderModeChange();

	void renderParticles(std::shared_ptr<renderer::Framebuffer> framebuffer,
		std::shared_ptr<renderer::RenderTargetTexture> paramTexture, 
		const std::vector<genericfsim::manager::SimulationManager::ParticleGfxData>& data);
};

} // namespace gfx3D