#include "headers/diffRenderProxy.h"
#include <armadillo>
#include <iostream>

using namespace visual;

DiffRendererProxy::DiffRendererProxy(std::shared_ptr<Renderer3DInterface> renderer3D)
	:renderer3D(std::static_pointer_cast<ParamInterface>(renderer3D)), renderEngine(renderer::RenderEngine::getInstance())
{
	referenceFramebuffer = std::make_shared<renderer::Framebuffer>(
		renderer::Framebuffer::toArray({ std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE),
			})
	);

	pertPlusFramebuffer = std::make_shared<renderer::Framebuffer>(
		renderer::Framebuffer::toArray({ std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE) })
	);

	pertMinusFramebuffer = std::make_shared<renderer::Framebuffer>(
		renderer::Framebuffer::toArray({ std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE) })
	);

	currentParamFramebuffer = std::make_shared<renderer::Framebuffer>(
		renderer::Framebuffer::toArray({ std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE) })
	);

	parameterSSBO = std::make_unique<renderer::StorageBuffer<glm::vec4>>(1000, GL_DYNAMIC_DRAW);
	perturbationSSBO = std::make_unique<renderer::StorageBuffer<glm::vec4>>(1000, GL_STATIC_DRAW);
	resultSSBO = std::make_unique<renderer::StorageBuffer<ResultSSBOData>>(1000, GL_DYNAMIC_READ);
	perturbationProgram = std::make_unique<renderer::ComputeProgram>("shaders/diffRender/perturbation.comp");
	parameterSSBO->bindBuffer(0);
	perturbationSSBO->bindBuffer(1);
	resultSSBO->bindBuffer(2);
	
	stochaisticGradientSSBO = std::make_unique<renderer::StorageBuffer<glm::vec4>>(1000, GL_DYNAMIC_READ);
	stochaisticGradientProgram = std::make_unique<renderer::ShaderProgram>("shaders/3D/util/quad.vert", "shaders/3D/diffRender/stochGradient.frag");
	(*stochaisticGradientProgram)["referenceImage"] = *referenceFramebuffer->getColorAttachments()[0];
	(*stochaisticGradientProgram)["plusPertImage"] = *pertPlusFramebuffer->getColorAttachments()[0];
	(*stochaisticGradientProgram)["minusPertImage"] = *pertMinusFramebuffer->getColorAttachments()[0];
	stochaisticGradientSSBO->bindBuffer(10);

	showQuad = std::make_unique<renderer::Square>();
	showProgram = std::make_unique<renderer::ShaderProgram>("shaders/3D/util/quad.vert", "shaders/3D/util/quad.frag");

	addParamLine({ &showSim, &updateReference, &updateParams });
	addParamLine({ &showReference, &randomizeParams, &adamEnabled, &resetAdamButton });
	addParamLine({ &speedPerturbation, &stochaisticGradientSamples });
	addParamLine({ &posPerturbation });
}

void DiffRendererProxy::render(std::shared_ptr<renderer::Framebuffer> framebuffer, const Gfx3DRenderData& data)
{
	if (updateReference.value)
	{
		renderer3D->render(referenceFramebuffer, data);
	}

	if (updateParams.value || paramData.particleData.size() != data.particleData.size())
	{
		paramData = paramDataTmp = data;
		initParameterAndPerturbationSSBO(data);
		updateSSBOFromParams(data);
		resetAdam();
	}

	if (randomizeParams.value)
	{
		randomizeParamValues();
		updateSSBOFromParams(paramData);
		resetAdam();
	}

	if (resetAdamButton.value)
	{
		initParameterAndPerturbationSSBO(data);
		resetAdam();
	}

	if(showSim.value)
	{
		renderer3D->render(framebuffer, data);
	}
	else if (showReference.value)
	{
		copytextureToFramebuffer(*referenceFramebuffer->getColorAttachments()[0], framebuffer);
	}
	else
	{
		resetStochaisticGradientSSBO();
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		for (unsigned int i = 0; i < stochaisticGradientSamples.value; i++)
		{
			computePerturbation();
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			perturbateAndRenderParams();
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
			computeStochaisticGradient();
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		if (adamEnabled.value)
			runAdamIteration();

		copytextureToFramebuffer(*currentParamFramebuffer->getColorAttachments()[0], framebuffer);
	}
}

void DiffRendererProxy::setConfigData(const ConfigData3D& data)
{
	renderer3D->setConfigData(data);

	if (data.screenSize != prevScreenSize)
	{
		prevScreenSize = data.screenSize;
		referenceFramebuffer->setSize(data.screenSize);
		pertPlusFramebuffer->setSize(data.screenSize);
		pertMinusFramebuffer->setSize(data.screenSize);
		currentParamFramebuffer->setSize(data.screenSize);
	}

	if (data != prevConfigData)
	{
		prevConfigData = data;
		configChanged = true;
	}
}

void DiffRendererProxy::show(int screenWidth)
{
	ImGui::SeparatorText("DiffRendererProxy");
	ParamLineCollection::show(screenWidth);
	renderer3D->show(screenWidth);
}

void visual::DiffRendererProxy::initParameterAndPerturbationSSBO(const Gfx3DRenderData& data)
{
	unsigned int paramSize = data.particleData.size();
	perturbationSSBO->setSize(paramSize);
	parameterSSBO->setSize(paramSize);
	resultSSBO->setSize(paramSize);
	stochaisticGradientSSBO->setSize(paramSize);

	perturbationSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	for (unsigned int i = 0; i < paramSize; i++)
	{
		(*perturbationSSBO)[i] = glm::vec4(posPerturbation.value, posPerturbation.value, posPerturbation.value, speedPerturbation.value);
		//(*perturbationSSBO)[i] = glm::vec4(0.0f, 0.0f, 0.0f, speedPerturbation.value);
	}
	perturbationSSBO->unmapBuffer();
}

void visual::DiffRendererProxy::resetStochaisticGradientSSBO()
{
	stochaisticGradientSSBO->fillWithZeros();
}

void visual::DiffRendererProxy::computePerturbation()
{
	(*perturbationProgram)["seed"] = std::rand() % 1000;
	perturbationProgram->dispatchCompute(parameterSSBO->getSize() / 64 + 1, 1, 1);
}

void visual::DiffRendererProxy::computeStochaisticGradient()
{
	renderEngine.setViewport(0, 0, referenceFramebuffer->getSize().x, referenceFramebuffer->getSize().y);
	renderEngine.enableDepthTest(false);
	stochaisticGradientProgram->activate();
	(*stochaisticGradientProgram)["multiplier"] = 1.0f / stochaisticGradientSamples.value;
	(*stochaisticGradientProgram)["screenSize"] = referenceFramebuffer->getSize();
	renderer3D->getParamBufferOut()->bindBuffer(30);
	showQuad->draw();
}

void visual::DiffRendererProxy::perturbateAndRenderParams()
{
	paramDataTmp = paramData;
	resultSSBO->mapBuffer(0, -1, GL_MAP_READ_BIT);
	for (unsigned int i = 0; i < parameterSSBO->getSize(); i++)
	{
		paramDataTmp.particleData[i].v = (*resultSSBO)[i].paramPositiveOffset.w;
		paramDataTmp.particleData[i].pos.x = (*resultSSBO)[i].paramPositiveOffset.x;
		paramDataTmp.particleData[i].pos.y = (*resultSSBO)[i].paramPositiveOffset.y;
		paramDataTmp.particleData[i].pos.z = (*resultSSBO)[i].paramPositiveOffset.z;
	}
	renderer3D->render(pertPlusFramebuffer, paramDataTmp);
	for (unsigned int i = 0; i < parameterSSBO->getSize(); i++)
	{
		paramDataTmp.particleData[i].v = (*resultSSBO)[i].paramNegativeOffset.w;
		paramDataTmp.particleData[i].pos.x = (*resultSSBO)[i].paramNegativeOffset.x;
		paramDataTmp.particleData[i].pos.y = (*resultSSBO)[i].paramNegativeOffset.y;
		paramDataTmp.particleData[i].pos.z = (*resultSSBO)[i].paramNegativeOffset.z;
	}
	resultSSBO->unmapBuffer();
	renderer3D->render(pertMinusFramebuffer, paramDataTmp);
	
	parameterSSBO->mapBuffer(0, -1, GL_MAP_READ_BIT);
	for (unsigned int i = 0; i < parameterSSBO->getSize(); i++)
	{
		paramDataTmp.particleData[i].v = (*parameterSSBO)[i].w;
		paramDataTmp.particleData[i].pos.x = (*parameterSSBO)[i].x;
		paramDataTmp.particleData[i].pos.y = (*parameterSSBO)[i].y;
		paramDataTmp.particleData[i].pos.z = (*parameterSSBO)[i].z;
	}
	parameterSSBO->unmapBuffer();
	renderer3D->render(currentParamFramebuffer, paramDataTmp);
}

void visual::DiffRendererProxy::updateSSBOFromParams(const Gfx3DRenderData& data)
{
	unsigned int paramSize = data.particleData.size();
	parameterSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	currentParams = arma::fvec(paramSize * 4);
	for (unsigned int i = 0; i < paramSize; i++)
	{
		(*parameterSSBO)[i] = glm::vec4(data.particleData[i].pos, data.particleData[i].v);
		currentParams(i * 4) = data.particleData[i].pos.x;
		currentParams(i * 4 + 1) = data.particleData[i].pos.y;
		currentParams(i * 4 + 2) = data.particleData[i].pos.z;
		currentParams(i * 4 + 3) = data.particleData[i].v;
	}
	parameterSSBO->unmapBuffer();
}

void visual::DiffRendererProxy::randomizeParamValues()
{
	unsigned int paramSize = paramData.particleData.size();
	for (unsigned int i = 0; i < paramSize; i++)
	{
		paramData.particleData[i].v = float(std::rand()) / RAND_MAX * 5.0f;
	}
}

void visual::DiffRendererProxy::resetAdam()
{
	mVec = arma::fvec(currentParams.size(), arma::fill::zeros);
	vVec = arma::fvec(currentParams.size(), arma::fill::zeros);
	gradientVec = arma::fvec(currentParams.size(), arma::fill::zeros);
	gradientVecIndex = 0;
	t = 0;
}

void visual::DiffRendererProxy::runAdamIteration()
{
	stochaisticGradientSSBO->mapBuffer(0, -1, GL_MAP_READ_BIT);
	arma::fvec g(stochaisticGradientSSBO->getSize() * 4);
	for (unsigned int i = 0; i < stochaisticGradientSSBO->getSize(); i++)
	{
		g(i * 4) = (*stochaisticGradientSSBO)[i].x;
		g(i * 4 + 1) = (*stochaisticGradientSSBO)[i].y;
		g(i * 4 + 2) = (*stochaisticGradientSSBO)[i].z;
		g(i * 4 + 3) = (*stochaisticGradientSSBO)[i].w;
		/*if ((*stochaisticGradientSSBO)[i].x != 0.0f)
		{
			std::cout << "x is not zero" << std::endl;
		}*/
	}
	stochaisticGradientSSBO->unmapBuffer();
	//std::cout << "g: " << g << std::endl;

	gradientVec += g;
	gradientVecIndex++;
	if (gradientVecIndex < 10)
	{
		return;
	}
	gradientVecIndex = 0;
	g = gradientVec / 10.0f;
	gradientVec = arma::fvec(currentParams.size(), arma::fill::zeros);

	constexpr float alfa = 0.8f;
	constexpr float beta1 = 0.9f;
	constexpr float beta2 = 0.999f;
	constexpr float epsilon = 1e-8f;

	t += 1.0f;
	mVec = beta1 * mVec + (1.0f - beta1) * g;
	vVec = beta2 * vVec + (1.0f - beta2) * arma::square(g);
	arma::fvec mHat = mVec / (1.0f - std::pow(beta1, t));
	arma::fvec vHat = vVec / (1.0f - std::pow(beta2, t));
	currentParams -= alfa * mHat / (arma::sqrt(vHat) + epsilon);

	parameterSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	for (unsigned int i = 0; i < parameterSSBO->getSize(); i++)
	{
		(*parameterSSBO)[i] = glm::vec4(currentParams(i * 4), currentParams(i * 4 + 1), currentParams(i * 4 + 2), currentParams(i * 4 + 3));
	}
	parameterSSBO->unmapBuffer();
}

void visual::DiffRendererProxy::copytextureToFramebuffer(const renderer::Texture& texture, std::shared_ptr<renderer::Framebuffer> framebuffer) const
{
	framebuffer->bind();
	renderEngine.setViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
	renderEngine.enableDepthTest(false);
	showProgram->activate();
	(*showProgram)["colorTexture"] = texture;
	showQuad->draw();
}
