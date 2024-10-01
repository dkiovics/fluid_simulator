#pragma once

#include <memory>
#include <engine/framebuffer.h>
#include <compute/computeTexture.h>
#include <compute/computeProgram.h>
#include <compute/storageBuffer.h>
#include <geometries/basicGeometries.h>
#include <armadillo>
#include "gfx3D/renderer3DInterface.h"
#include "paramInterface.h"

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

	std::shared_ptr<ParamInterface> renderer3D;
	renderer::RenderEngine& renderEngine;

	Gfx3DRenderData paramData;
	Gfx3DRenderData paramDataTmp;
	ConfigData3D prevConfigData;
	bool configChanged = true;

	arma::fvec currentParams;
	arma::fvec mVec;
	arma::fvec vVec;
	arma::fvec gradientVec;
	int gradientVecIndex = 0;
	float t = 0;

	std::shared_ptr<renderer::Framebuffer> referenceFramebuffer;

	std::shared_ptr<renderer::Framebuffer> pertPlusFramebuffer;
	std::shared_ptr<renderer::Framebuffer> pertMinusFramebuffer;
	std::shared_ptr<renderer::Framebuffer> currentParamFramebuffer;

	ParamFloat speedPerturbation = ParamFloat("Speed perturbation", 0.2f, 0.01f, 5.0f);
	ParamFloat posPerturbation = ParamFloat("Pos perturbation", 0.05f, 0.0f, 0.5f);
	ParamInt stochaisticGradientSamples = ParamInt("Stochaistic gradient samples", 1, 1, 10);
	ParamBool showSim = ParamBool("Show simulation", false);
	ParamButton updateReference = ParamButton("Update reference");
	ParamButton updateParams = ParamButton("Update params");
	ParamButton randomizeParams = ParamButton("Randomize params");
	ParamBool showReference = ParamBool("Show reference", false);
	ParamBool adamEnabled = ParamBool("Adam enabled", false);
	ParamButton resetAdamButton = ParamButton("Reset Adam");

	struct ResultSSBOData
	{
		glm::vec4 signedPerturbation;
		glm::vec4 paramPositiveOffset;
		glm::vec4 paramNegativeOffset;
	};
	std::unique_ptr<renderer::StorageBuffer<glm::vec4>> parameterSSBO;
	std::unique_ptr<renderer::StorageBuffer<glm::vec4>> perturbationSSBO;
	std::unique_ptr<renderer::StorageBuffer<ResultSSBOData>> resultSSBO;

	std::unique_ptr<renderer::ComputeProgram> perturbationProgram;

	std::unique_ptr<renderer::StorageBuffer<glm::vec4>> stochaisticGradientSSBO;
	std::unique_ptr<renderer::ShaderProgram> stochaisticGradientProgram;

	std::unique_ptr<renderer::Square> showQuad;
	std::unique_ptr<renderer::ShaderProgram> showProgram;

	void initParameterAndPerturbationSSBO(const Gfx3DRenderData& data);

	void resetStochaisticGradientSSBO();
	void computePerturbation();
	void computeStochaisticGradient();
	
	void perturbateAndRenderParams();

	void updateSSBOFromParams(const Gfx3DRenderData& data);
	void randomizeParamValues();

	void resetAdam();
	void runAdamIteration();

	void copytextureToFramebuffer(const renderer::Texture& texture, std::shared_ptr<renderer::Framebuffer> framebuffer) const;
};

} // namespace visual