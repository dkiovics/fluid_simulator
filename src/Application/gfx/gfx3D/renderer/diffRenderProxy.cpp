#include "diffRenderProxy.h"
#include <armadillo>
#include <iostream>

using namespace gfx3D;

DiffRendererProxy::DiffRendererProxy(std::shared_ptr<Renderer3DInterface> renderer3D)
	:renderer3D(renderer3D), renderEngine(renderer::RenderEngine::getInstance())
{
	referenceFramebuffer = std::make_shared<renderer::Framebuffer>(
		renderer::Framebuffer::toArray({ std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE),
			})
	);

	paramFramebuffer = std::make_shared<renderer::Framebuffer>(
		renderer::Framebuffer::toArray({ std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT) }),
		nullptr
	);

	pertPlusFramebuffer = std::make_shared<renderer::Framebuffer>(
		renderer::Framebuffer::toArray({ std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE) })
	);

	pertMinusFramebuffer = std::make_shared<renderer::Framebuffer>(
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
	stochaisticGradientProgram = std::make_unique<renderer::ShaderProgram>("shaders/quad.vert", "shaders/diffRender/stochGradient.frag");
	(*stochaisticGradientProgram)["referenceImage"] = *referenceFramebuffer->getColorAttachments()[0];
	(*stochaisticGradientProgram)["plusPertImage"] = *pertPlusFramebuffer->getColorAttachments()[0];
	(*stochaisticGradientProgram)["minusPertImage"] = *pertMinusFramebuffer->getColorAttachments()[0];
	(*stochaisticGradientProgram)["contributionImage"] = *paramFramebuffer->getColorAttachments()[0];
	stochaisticGradientSSBO->bindBuffer(10);

	showQuad = std::make_unique<renderer::Square>();
	showProgram = std::make_unique<renderer::ShaderProgram>("shaders/quad.vert", "shaders/quad.frag");
	showIntProgram = std::make_unique<renderer::ShaderProgram>("shaders/quad.vert", "shaders/quad_int.frag");

	addParamLine({ &updateReference, &updateReferenceButton });
	addParamLine({ &speedPerturbation, &showReference });
	addParamLine({ &adamEnabled });
}

void DiffRendererProxy::render(std::shared_ptr<renderer::Framebuffer> framebuffer, std::shared_ptr<renderer::RenderTargetTexture>, const Gfx3DRenderData& data)
{
	if (showReference.value)
	{
		copytextureToFramebuffer(*referenceFramebuffer->getColorAttachments()[0], framebuffer);
		return;
	}
	if (updateReference.value || updateReferenceButton.value || configChanged)
	{
		configChanged = false;
		paramDataTmp = data;
		renderReferenceImage(data);
		initParameterAndPerturbationSSBO(data);
		resetAdam();
		copytextureToFramebuffer(*referenceFramebuffer->getColorAttachments()[0], framebuffer);
		/*framebuffer->bind();
		renderEngine.setViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
		renderEngine.enableDepthTest(false);
		showIntProgram->activate();
		(*showIntProgram)["colorTexture"] = *paramFramebuffer->getColorAttachments()[0];
		showQuad->draw();*/
	}
	else
	{
		computePerturbation();
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		perturbateAndRenderParams();
		computeStochaisticGradient();
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		if(adamEnabled.value)
			runAdamIteration();
		copytextureToFramebuffer(*pertPlusFramebuffer->getColorAttachments()[0], framebuffer);
	}
}

void DiffRendererProxy::setConfigData(const ConfigData3D& data)
{
	renderer3D->setConfigData(data);

	if (data.screenSize != prevScreenSize)
	{
		prevScreenSize = data.screenSize;
		referenceFramebuffer->setSize(data.screenSize);
		paramFramebuffer->setSize(data.screenSize);
		pertPlusFramebuffer->setSize(data.screenSize);
		pertMinusFramebuffer->setSize(data.screenSize);
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

void gfx3D::DiffRendererProxy::initParameterAndPerturbationSSBO(const Gfx3DRenderData& data)
{
	unsigned int paramSize = data.particleData.size();
	perturbationSSBO->setSize(paramSize);
	parameterSSBO->setSize(paramSize);
	resultSSBO->setSize(paramSize);
	stochaisticGradientSSBO->setSize(paramSize);

	perturbationSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	for (unsigned int i = 0; i < paramSize; i++)
	{
		(*perturbationSSBO)[i] = glm::vec4(speedPerturbation.value, 0.0f, 0.0f, 0.0f);	
	}
	perturbationSSBO->unmapBuffer();
	parameterSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	currentParams = arma::fvec(paramSize * 4);
	for (unsigned int i = 0; i < paramSize; i++)
	{
		(*parameterSSBO)[i] = glm::vec4(float(std::rand()) / RAND_MAX * 5.0f, 0.0f, 0.0f, 0.0f);
		currentParams(i * 4) = (*parameterSSBO)[i].x;
	}
	parameterSSBO->unmapBuffer();
}

void gfx3D::DiffRendererProxy::computePerturbation()
{
	(*perturbationProgram)["seed"] = std::rand() % 1000;
	perturbationProgram->dispatchCompute(parameterSSBO->getSize() / 64 + 1, 1, 1);
}

void gfx3D::DiffRendererProxy::computeStochaisticGradient()
{
	stochaisticGradientSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	for (unsigned int i = 0; i < stochaisticGradientSSBO->getSize(); i++)
	{
		(*stochaisticGradientSSBO)[i] = glm::vec4(0.0f);
	}
	stochaisticGradientSSBO->unmapBuffer();
	renderEngine.setViewport(0, 0, referenceFramebuffer->getSize().x, referenceFramebuffer->getSize().y);
	renderEngine.enableDepthTest(false);
	stochaisticGradientProgram->activate();
	showQuad->draw();
}

void gfx3D::DiffRendererProxy::renderReferenceImage(const Gfx3DRenderData& data) const
{
	paramFramebuffer->bind();
	renderEngine.setViewport(0, 0, paramFramebuffer->getSize().x, paramFramebuffer->getSize().y);
	renderEngine.clearViewport(glm::vec4(0, 0, 0, 0));
	renderer3D->render(referenceFramebuffer, paramFramebuffer->getColorAttachments()[0], data);
}

void gfx3D::DiffRendererProxy::perturbateAndRenderParams()
{
	resultSSBO->mapBuffer(0, -1, GL_MAP_READ_BIT);
	for (unsigned int i = 0; i < parameterSSBO->getSize(); i++)
	{
		paramDataTmp.particleData[i].v = (*resultSSBO)[i].paramPositiveOffset.x;
	}
	paramFramebuffer->bind();
	renderEngine.setViewport(0, 0, paramFramebuffer->getSize().x, paramFramebuffer->getSize().y);
	renderEngine.clearViewport(glm::vec4(0, 0, 0, 0));
	renderer3D->render(pertPlusFramebuffer, paramFramebuffer->getColorAttachments()[0], paramDataTmp);
	for (unsigned int i = 0; i < parameterSSBO->getSize(); i++)
	{
		paramDataTmp.particleData[i].v = (*resultSSBO)[i].paramNegativeOffset.x;
	}
	resultSSBO->unmapBuffer();
	renderer3D->render(pertMinusFramebuffer, nullptr, paramDataTmp);
}

void gfx3D::DiffRendererProxy::resetAdam()
{
	mVec = arma::fvec(currentParams.size(), arma::fill::zeros);
	vVec = arma::fvec(currentParams.size(), arma::fill::zeros);
	gradientVec = arma::fvec(currentParams.size(), arma::fill::zeros);
	gradientVecIndex = 0;
	t = 0;
}

void gfx3D::DiffRendererProxy::runAdamIteration()
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

void gfx3D::DiffRendererProxy::copytextureToFramebuffer(const renderer::Texture& texture, std::shared_ptr<renderer::Framebuffer> framebuffer) const
{
	framebuffer->bind();
	renderEngine.setViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
	renderEngine.enableDepthTest(false);
	showProgram->activate();
	(*showProgram)["colorTexture"] = texture;
	showQuad->draw();
}
