#pragma once

#include <memory>
#include <engine/framebuffer.h>
#include <compute/computeTexture.h>
#include <compute/computeProgram.h>
#include <compute/storageBuffer.h>
#include <geometries/basicGeometries.h>
#include "gfx3D/renderer3DInterface.h"
#include "gfx3D/renderer/headers/paramInterface.h"
#include "gfx3D/optimizer/adam.h"
#include "gfx3D/optimizer/densityControl.h"
#include "gradientCalculatorInterface.h"

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

	std::shared_ptr<ParamInterface> renderer3D;
	renderer::RenderEngine& renderEngine;

	std::unique_ptr<AdamOptimizer> adam;
	std::unique_ptr<DensityControl> densityControl;

	std::unique_ptr<GradientCalculatorInterface> gradientCalculator;

	renderer::fb_ptr referenceFramebuffer;
	renderer::fb_ptr currentParamFramebuffer;

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

	renderer::ssbo_ptr<float> particleMovementAbsSSBO;

	std::unique_ptr<renderer::Square> showQuad;
	std::shared_ptr<renderer::ShaderProgram> showProgram;

	void reset(renderer::ssbo_ptr<ParticleShaderData> data);
	void randomizeParamValues(renderer::ssbo_ptr<ParticleShaderData> baselineData);

	void updateOptimizedParams(renderer::ssbo_ptr<ParticleShaderData> data);

	void copytextureToFramebuffer(const renderer::Texture& texture, std::shared_ptr<renderer::Framebuffer> framebuffer) const;

	void pushApartOptimizedParams();

	void updateParticleDensities();

	void updateSimulator();
};

} // namespace visual