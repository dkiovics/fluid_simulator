#pragma once

#include <memory>
#include <engine/framebuffer.h>
#include <compute/computeTexture.h>
#include <compute/computeProgram.h>
#include <compute/storageBuffer.h>
#include <geometries/basicGeometries.h>
#include "gfx3D/renderer3DInterface.h"
#include "paramInterface.h"
#include "gfx3D/optimizer/adam.h"
#include "gfx3D/optimizer/densityControl.h"

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

	renderer::fb_ptr referenceFramebuffer;
	renderer::fb_ptr pertPlusFramebuffer;
	renderer::fb_ptr pertMinusFramebuffer;
	renderer::fb_ptr currentParamFramebuffer;

	ParamFloat speedPerturbation = ParamFloat("Speed perturbation", 0.2f, 0.0f, 5.0f);
	ParamFloat posPerturbation = ParamFloat("Pos perturbation", 0.05f, 0.0f, 0.5f);
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
	ParamBool useDepthImage = ParamBool("Use depth image", false);
	ParamFloat depthErrorScale = ParamFloat("Depth error scale", 1.0f, 0.0f, 20.0f);
	ParamBool enableDensityControl = ParamBool("Enable density control", false);
	ParamBool showMovementAbs = ParamBool("Show avg movement", false);
	ParamButton updateSimulatorButton = ParamButton("Update simulator");
	ParamBool updateDensities = ParamBool("Update densities", false);

	renderer::ssbo_ptr<ParticleShaderData> perturbationPresetSSBO;
	renderer::ssbo_ptr<ParticleShaderData> paramNegativeOffsetSSBO;
	renderer::ssbo_ptr<ParticleShaderData> paramPositiveOffsetSSBO;
	renderer::ssbo_ptr<ParticleShaderData> optimizedParamsSSBO;
	renderer::ssbo_ptr<float> particleMovementAbsSSBO;

	renderer::ssbo_ptr<float> stochaisticGradientSSBO;

	renderer::compute_ptr perturbationProgram;
	renderer::compute_ptr stochaisticColorGradientProgram;
	renderer::compute_ptr stochaisticDepthGradientProgram;
	renderer::compute_ptr particleDataToFloatProgram;
	renderer::compute_ptr floatToParticleDataProgram;

	std::unique_ptr<renderer::Square> showQuad;
	std::shared_ptr<renderer::ShaderProgram> showProgram;

	void reset(renderer::ssbo_ptr<ParticleShaderData> data);
	void randomizeParamValues(renderer::ssbo_ptr<ParticleShaderData> baselineData);

	void computePerturbation();
	void computeStochaisticGradient();

	void updateAdamParams(renderer::ssbo_ptr<ParticleShaderData> data);
	void updateOptimizedParamsFromAdam();
	
	void renderParams();

	void copytextureToFramebuffer(const renderer::Texture& texture, std::shared_ptr<renderer::Framebuffer> framebuffer) const;

	void pushApartOptimizedParams();

	void updateParticleDensities();

	void updateSimulator();
};

} // namespace visual