#include "headers/diffRenderProxy.h"
#include <armadillo>
#include <iostream>

using namespace visual;

DiffRendererProxy::DiffRendererProxy(std::shared_ptr<Renderer3DInterface> renderer3D)
	:renderer3D(std::static_pointer_cast<ParamInterface>(renderer3D)), renderEngine(renderer::RenderEngine::getInstance())
{
	referenceFramebuffer = renderer::make_fb(
		renderer::Framebuffer::toArray({ 
			renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE),
		})
	);

	pertPlusFramebuffer = renderer::make_fb(
		renderer::Framebuffer::toArray({ 
			renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE) 
		})
	);

	pertMinusFramebuffer = std::make_shared<renderer::Framebuffer>(
		renderer::Framebuffer::toArray({ 
			renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE) 
		})
	);

	currentParamFramebuffer = std::make_shared<renderer::Framebuffer>(
		renderer::Framebuffer::toArray({ 
			renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE) 
		})
	);

	perturbationPresetSSBO = renderer::make_ssbo<ParticleShaderData>(1000, GL_STATIC_DRAW);
	paramNegativeOffsetSSBO = renderer::make_ssbo<ParticleShaderData>(1000, GL_DYNAMIC_COPY);
	paramPositiveOffsetSSBO = renderer::make_ssbo<ParticleShaderData>(1000, GL_DYNAMIC_COPY);
	optimizedParamsSSBO = renderer::make_ssbo<ParticleShaderData>(1000, GL_DYNAMIC_COPY);
	stochaisticGradientSSBO = renderer::make_ssbo<float>(1000, GL_DYNAMIC_COPY);

	perturbationProgram = renderer::make_compute("shaders/3D/diffRender/perturbation.comp");
	stochaisticGradientProgram = renderer::make_compute("shaders/3D/diffRender/stochGradient.comp");
	particleDataToFloatProgram = renderer::make_compute("shaders/3D/diffRender/particleDataToFloat.comp");
	floatToParticleDataProgram = renderer::make_compute("shaders/3D/diffRender/floatToParticleData.comp");

	(*stochaisticGradientProgram)["referenceImage"] = *referenceFramebuffer->getColorAttachments()[0];
	(*stochaisticGradientProgram)["plusPertImage"] = *pertPlusFramebuffer->getColorAttachments()[0];
	(*stochaisticGradientProgram)["minusPertImage"] = *pertMinusFramebuffer->getColorAttachments()[0];

	showQuad = std::make_unique<renderer::Square>();
	showProgram = renderer::make_shader("shaders/3D/util/quad.vert", "shaders/3D/util/quad.frag");

	adam = std::make_unique<AdamOptimizer>(1);

	addParamLine({ &showSim, &updateReference, &updateParams });
	addParamLine({ &showReference, &randomizeParams, &adamEnabled, &resetAdamButton });
	addParamLine({ &speedPerturbation });
	addParamLine({ &posPerturbation });
}

void DiffRendererProxy::render(renderer::fb_ptr framebuffer, renderer::ssbo_ptr<ParticleShaderData> data)
{
	renderEngine.setViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);

	if(updateReference.value)
	{
		renderer3D->render(referenceFramebuffer, data);
	}

	if(data->getSize() != paramNegativeOffsetSSBO->getSize())
	{
		paramNegativeOffsetSSBO->setSize(data->getSize());
		paramPositiveOffsetSSBO->setSize(data->getSize());
		optimizedParamsSSBO->setSize(data->getSize());
		perturbationPresetSSBO->setSize(data->getSize());
		stochaisticGradientSSBO->setSize(data->getSize() * ParticleShaderData::paramCount);
		adam->setParamNum(data->getSize() * ParticleShaderData::paramCount);
		reset(data);
	}

	if(randomizeParams.value)
	{
		randomizeParamValues(data);
	}

	if(resetAdamButton.value)
	{
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		updateAdamParams(optimizedParamsSSBO);
	}

	if(updateParams.value)
	{
		reset(data);
	}

	if (showSim.value)
	{
		renderer3D->render(framebuffer, data);
	}
	else if (showReference.value)
	{
		copytextureToFramebuffer(*referenceFramebuffer->getColorAttachments()[0], framebuffer);
	}
	else
	{
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		updateOptimizedParamsFromAdam();
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		computePerturbation();
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		renderParams();

		if(adamEnabled.value)
		{
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			computeStochaisticGradient();
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			adam->updateGradient(stochaisticGradientSSBO);
		}

		copytextureToFramebuffer(*currentParamFramebuffer->getColorAttachments()[0], framebuffer);
	}
}

void DiffRendererProxy::setConfigData(const ConfigData3D& data)
{
	renderer3D->setConfigData(data);

	referenceFramebuffer->setSize(data.screenSize);
	pertPlusFramebuffer->setSize(data.screenSize);
	pertMinusFramebuffer->setSize(data.screenSize);
	currentParamFramebuffer->setSize(data.screenSize);
}

void DiffRendererProxy::show(int screenWidth)
{
	ImGui::SeparatorText("DiffRendererProxy");
	ParamLineCollection::show(screenWidth);
	ImGui::SeparatorText("Adam");
	adam->show(screenWidth * 2);
	renderer3D->show(screenWidth);
}

void visual::DiffRendererProxy::reset(renderer::ssbo_ptr<ParticleShaderData> data)
{
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
	data->mapBuffer(0, -1, GL_MAP_READ_BIT);
	optimizedParamsSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	memcpy(&(*optimizedParamsSSBO)[0], &(*data)[0], data->getSize() * sizeof(ParticleShaderData));
	optimizedParamsSSBO->unmapBuffer();
	data->unmapBuffer();
	perturbationPresetSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	unsigned int paramNum = perturbationPresetSSBO->getSize();
	for (unsigned int i = 0; i < paramNum; i++)
	{
		(*perturbationPresetSSBO)[i].posAndSpeed = 
			glm::vec4(posPerturbation.value, posPerturbation.value, posPerturbation.value, speedPerturbation.value);
		(*perturbationPresetSSBO)[i].density = glm::vec4(0.0f);
	}
	perturbationPresetSSBO->unmapBuffer();
	updateAdamParams(data);
}

void visual::DiffRendererProxy::randomizeParamValues(renderer::ssbo_ptr<ParticleShaderData> baselineData)
{
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
	unsigned int paramNum = optimizedParamsSSBO->getSize();
	optimizedParamsSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	baselineData->mapBuffer(0, -1, GL_MAP_READ_BIT);
	for (unsigned int i = 0; i < paramNum; i++)
	{
		(*optimizedParamsSSBO)[i] = (*baselineData)[i];
		(*optimizedParamsSSBO)[i].posAndSpeed.w = float(std::rand()) / RAND_MAX * 20.0f;
	}
	optimizedParamsSSBO->unmapBuffer();
	baselineData->unmapBuffer();
	updateAdamParams(optimizedParamsSSBO);
}

void visual::DiffRendererProxy::computePerturbation()
{
	(*perturbationProgram)["seed"] = std::rand() % 1000;
	optimizedParamsSSBO->bindBuffer(0);
	perturbationPresetSSBO->bindBuffer(1);
	paramNegativeOffsetSSBO->bindBuffer(2);
	paramPositiveOffsetSSBO->bindBuffer(3);
	perturbationProgram->dispatchCompute(optimizedParamsSSBO->getSize() / 64 + 1, 1, 1);
}

void visual::DiffRendererProxy::computeStochaisticGradient()
{
	paramNegativeOffsetSSBO->bindBuffer(0);
	paramPositiveOffsetSSBO->bindBuffer(1);
	renderer3D->getParamBufferOut()->bindBuffer(2);
	stochaisticGradientSSBO->fillWithZeros();
	stochaisticGradientSSBO->bindBuffer(3);
	(*stochaisticGradientProgram)["screenSize"] = referenceFramebuffer->getSize();
	stochaisticGradientProgram->dispatchCompute(referenceFramebuffer->getSize().x, referenceFramebuffer->getSize().y, 1);
}

void visual::DiffRendererProxy::updateAdamParams(renderer::ssbo_ptr<ParticleShaderData> data)
{
	renderer::ssbo_ptr<float> floatData = renderer::make_ssbo<float>(data->getSize() * ParticleShaderData::paramCount, GL_DYNAMIC_COPY);
	data->bindBuffer(0);
	floatData->bindBuffer(1);
	particleDataToFloatProgram->dispatchCompute(data->getSize() / 64 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	adam->reset(floatData);
}

void visual::DiffRendererProxy::updateOptimizedParamsFromAdam()
{
	auto floatData = adam->getOptimizedFloatData();
	floatData->bindBuffer(0);
	optimizedParamsSSBO->bindBuffer(1);
	floatToParticleDataProgram->dispatchCompute(optimizedParamsSSBO->getSize() / 64 + 1, 1, 1);
}

void visual::DiffRendererProxy::renderParams()
{
	renderer3D->invalidateParamBuffer();
	currentParamFramebuffer->bind();
	renderEngine.clearViewport(glm::vec4(0.0f), 1.0f);
	renderer3D->render(currentParamFramebuffer, optimizedParamsSSBO);

	pertPlusFramebuffer->bind();
	renderEngine.clearViewport(glm::vec4(0.0f), 1.0f);
	renderer3D->render(pertPlusFramebuffer, paramPositiveOffsetSSBO);

	pertMinusFramebuffer->bind();
	renderEngine.clearViewport(glm::vec4(0.0f), 1.0f);
	renderer3D->render(pertMinusFramebuffer, paramNegativeOffsetSSBO);
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

