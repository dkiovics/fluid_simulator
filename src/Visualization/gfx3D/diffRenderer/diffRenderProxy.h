#pragma once

#include <memory>
#include <engine/framebuffer.h>
#include <compute/computeTexture.h>
#include <compute/computeProgram.h>
#include <compute/storageBuffer.h>
#include <geometries/basicGeometries.h>
#include <engineUtils/object.h>
#include "gfx3D/renderer3DInterface.h"
#include "gfx3D/renderer/headers/paramInterface.h"
#include "gfx3D/optimizer/adam.h"
#include "gfx3D/optimizer/densityControl.h"
#include "gfx3D/renderer/headers/simulationGfx3DRenderer.h"
#include "gradientCalculatorInterface.h"
#include "gfx3D/gradientSmoothing/gradientSmoothing.h"
#include <optional>

namespace visual
{

class DiffRendererProxy : public Renderer3DInterface
{
public:
	DiffRendererProxy(std::shared_ptr<Renderer3DInterface> renderer);

	void render(renderer::fb_ptr framebuffer, renderer::ssbo_ptr<ParticleShaderData> data) override;
	void setConfigData(const ConfigData3D& data) override;

	void show(int screenWidth) override;

private:
	glm::ivec2 prevScreenSize = glm::ivec2(1000, 1000);

	ConfigData3D configData;

	std::shared_ptr<SimulationGfx3DRenderer> renderer3D;
	renderer::RenderEngine& renderEngine;

	std::unique_ptr<AdamOptimizer> adam;
	std::unique_ptr<DensityControl> densityControl;

	std::unique_ptr<GradientCalculatorInterface> gradientCalculator;
	std::unique_ptr<GradientSmoothing> gradientSmoothing;

	renderer::fb_ptr referenceFramebuffer;
	renderer::fb_ptr currentParamFramebuffer;

	std::optional<renderer::Camera3D::CameraData> backupCamera;

	ParamBool showSim = ParamBool("Show simulation", false);
	ParamButton updateReference = ParamButton("Update reference");
	ParamButton updateParams = ParamButton("Update params");
	ParamButton randomizeParams = ParamButton("Randomize params");
	ParamBool showReference = ParamBool("Show reference", false);
	ParamBool adamEnabled = ParamBool("Adam enabled", false);
	ParamButton resetAdamButton = ParamButton("Reset Adam");
	ParamBool autoPushApart = ParamBool("Auto push apart", false);
	ParamInt pushApartUpdatePeriod = ParamInt("Push apart update period", 80, 20, 1000);
	ParamButton pushApartButton = ParamButton("Push apart");
	ParamBool enableDensityControl = ParamBool("Enable density control", false);
	ParamButton updateSimulatorButton = ParamButton("Update simulator");
	ParamBool updateDensities = ParamBool("Update densities", false);
	ParamBool doSimulatorGradientCalc = ParamBool("Do simulator gradient calc", false);
	ParamBool gradientVisualization = ParamBool("Gradient visualization", false);
	ParamButton backupCameraPos = ParamButton("Backup camera pos");
	ParamButton restoreCameraPos = ParamButton("Restore camera pos");
	ParamFloat arrowDensityThreshold = ParamFloat("Arrow density threshold", 0.8f, 0.5f, 5.0f);
	ParamBool enableGradientSmoothing = ParamBool("Enable gradient smoothing", false);
	ParamFloat gradientSmoothingSphereR = ParamFloat("Gradient smoothing sphere R", 1.5f, 0.8f, 5.0f);

	renderer::ssbo_ptr<float> particleMovementAbsSSBO;
	renderer::ssbo_ptr<GradientCalculatorInterface::ParticleGradientData> particleGradientSSBO;
	bool particleGradientValid = false;
	bool newFluidParamsNeeded = true;

	std::unique_ptr<renderer::Square> showQuad;
	std::shared_ptr<renderer::ShaderProgram> showProgram;
	std::unique_ptr<renderer::InstancedGeometry> gradientArrows;
	renderer::shader_ptr gradientArrowShader;

	void reset(renderer::ssbo_ptr<ParticleShaderData> data);
	void randomizeParamValues(renderer::ssbo_ptr<ParticleShaderData> baselineData);

	void updateOptimizedParams(renderer::ssbo_ptr<ParticleShaderData> data);

	void copytextureToFramebuffer(const renderer::Texture& texture, std::shared_ptr<renderer::Framebuffer> framebuffer) const;

	void pushApartOptimizedParams();

	void updateParticleDensities();

	void updateSimulator();
};

} // namespace visual