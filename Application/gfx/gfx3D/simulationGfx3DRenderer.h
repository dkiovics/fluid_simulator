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
#include "renderer3DInterface.h"

namespace gfx3D
{

class SimulationGfx3DRenderer : public Renderer3DInterface
{
public:
	SimulationGfx3DRenderer(std::shared_ptr<renderer::RenderEngine> engine, std::shared_ptr<renderer::Camera3D> camera, 
		std::shared_ptr<renderer::Lights> lights, const std::vector<std::unique_ptr<renderer::Object3D<renderer::Geometry>>>& obstacleGfxArray,
		unsigned int maxParticleNum, ConfigData3D configData);

	void setConfigData(const ConfigData3D& configData) override;

	void render(std::shared_ptr<renderer::Framebuffer> framebuffer, const Gfx3DRenderData& renderData) const override;

private:
	ConfigData3D configData;

	std::shared_ptr<renderer::RenderEngine> engine;
	std::shared_ptr<renderer::Camera3D> camera;
	std::shared_ptr<renderer::Lights> lights;
	const std::vector<std::unique_ptr<renderer::Object3D<renderer::Geometry>>>& obstacleGfxArray;

	std::unique_ptr<renderer::Object3D<renderer::Geometry>> planeGfx;
	std::unique_ptr<renderer::Object3D<renderer::BasicGeometryArray>> ballsGfx;

	std::unique_ptr<TransparentBox> transparentBox;

	std::shared_ptr<renderer::GpuProgram> shaderProgramTextured;
	std::shared_ptr<renderer::GpuProgram> shaderProgramNotTextured;
	std::shared_ptr<renderer::GpuProgram> shaderProgramNotTexturedArray;

	std::shared_ptr<renderer::GpuProgram> showShaderProgram;
	std::shared_ptr<renderer::Square> showSquare;
	std::shared_ptr<renderer::RenderTargetTexture> renderTargetTexture;
	std::shared_ptr<renderer::Framebuffer> renderTargetFramebuffer;
};

} // namespace gfx3D