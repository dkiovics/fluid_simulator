#include "gradientCalculatorPos.h"

visual::GradientCalculatorPos::GradientCalculatorPos(std::shared_ptr<ParamInterface> renderer)
{
	renderer3D = renderer;

	perturbationProgram = renderer::make_compute("shaders/3D/diffRender/perturbation.comp");
	stochaisticColorGradientProgram = renderer::make_compute("shaders/3D/diffRender/stochGradient_color.comp");
	stochaisticDepthGradientProgram = renderer::make_compute("shaders/3D/diffRender/stochGradient_depth.comp");
	particleDataToFloatProgram = renderer::make_compute("shaders/3D/diffRender/particleDataToFloat.comp");
	floatToParticleDataProgram = renderer::make_compute("shaders/3D/diffRender/floatToParticleData.comp");

	(*stochaisticColorGradientProgram)["plusPertImage"] = *pertPlusFramebuffer->getColorAttachments()[0];
	(*stochaisticColorGradientProgram)["minusPertImage"] = *pertMinusFramebuffer->getColorAttachments()[0];
	(*stochaisticDepthGradientProgram)["plusPertImage"] = *pertPlusFramebuffer->getDepthAttachment();
	(*stochaisticDepthGradientProgram)["minusPertImage"] = *pertMinusFramebuffer->getDepthAttachment();

	auto camera = renderer3D->getCamera();
	camera->addProgram({ stochaisticDepthGradientProgram });
	camera->setUniformsForAllPrograms();

	addParamLine({ &speedAbsPerturbation });
	addParamLine({ &posPerturbation });
}

void visual::GradientCalculatorPos::updateOptimizedFloats(renderer::ssbo_ptr<float> data, renderer::ssbo_ptr<float> particleMovementAbs)
{
	if(data->getSize() != optimizedParamsSSBO->getSize() * ParticleShaderData::paramCount)
		throw std::runtime_error("GradientCalculatorPos::updateOptimizedFloats: data size does not match the optimizedParamsSSBO size");
	data->bindBuffer(0);
	optimizedParamsSSBO->bindBuffer(1);
	particleMovementAbs->bindBuffer(2);
	floatToParticleDataProgram->dispatchCompute(optimizedParamsSSBO->getSize() / 64 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void visual::GradientCalculatorPos::updateParticleParams(renderer::ssbo_ptr<ParticleShaderData> data)
{
	if (!perturbationPresetSSBO || perturbationPresetSSBO->getSize() != data->getSize())
	{
		perturbationPresetSSBO = renderer::make_ssbo<ParticleShaderData>(data->getSize(), GL_DYNAMIC_COPY);
		paramNegativeOffsetSSBO = renderer::make_ssbo<ParticleShaderData>(data->getSize(), GL_DYNAMIC_COPY);
		paramPositiveOffsetSSBO = renderer::make_ssbo<ParticleShaderData>(data->getSize(), GL_DYNAMIC_COPY);
		optimizedParamsSSBO = renderer::make_ssbo<ParticleShaderData>(data->getSize(), GL_DYNAMIC_COPY);
		stochaisticGradientSSBO = renderer::make_ssbo<float>(data->getSize() * ParticleShaderData::paramCount, GL_DYNAMIC_COPY);
	}
	data->copyTo(*optimizedParamsSSBO);
}

void visual::GradientCalculatorPos::reset()
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	perturbationPresetSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	unsigned int paramNum = perturbationPresetSSBO->getSize();
	for (unsigned int i = 0; i < paramNum; i++)
	{
		(*perturbationPresetSSBO)[i].posAndSpeed =
			glm::vec4(posPerturbation.value, posPerturbation.value, posPerturbation.value, speedAbsPerturbation.value);
		(*perturbationPresetSSBO)[i].density = glm::vec4(0.0f);
	}
	perturbationPresetSSBO->unmapBuffer();
}

renderer::ssbo_ptr<float> visual::GradientCalculatorPos::calculateGradient(renderer::fb_ptr referenceFramebuffer)
{
	pertPlusFramebuffer->setSize(referenceFramebuffer->getSize());
	pertMinusFramebuffer->setSize(referenceFramebuffer->getSize());

	(*perturbationProgram)["seed"] = std::rand() % 1000;
	optimizedParamsSSBO->bindBuffer(0);
	perturbationPresetSSBO->bindBuffer(1);
	paramNegativeOffsetSSBO->bindBuffer(2);
	paramPositiveOffsetSSBO->bindBuffer(3);
	perturbationProgram->dispatchCompute(optimizedParamsSSBO->getSize() / 64 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	pertPlusFramebuffer->bind();
	renderEngine.clearViewport(glm::vec4(0.0f), 1.0f);
	renderer3D->render(pertPlusFramebuffer, paramPositiveOffsetSSBO);
	
	pertMinusFramebuffer->bind();
	renderEngine.clearViewport(glm::vec4(0.0f), 1.0f);
	renderer3D->render(pertMinusFramebuffer, paramNegativeOffsetSSBO);

	paramNegativeOffsetSSBO->bindBuffer(0);
	paramPositiveOffsetSSBO->bindBuffer(1);
	renderer3D->getParamBufferOut()->bindBuffer(2);
	stochaisticGradientSSBO->fillWithZeros();
	stochaisticGradientSSBO->bindBuffer(3);
	if (useDepthImage.value)
	{
		(*stochaisticDepthGradientProgram)["referenceImage"] = *referenceFramebuffer->getDepthAttachment();
		(*stochaisticDepthGradientProgram)["screenSize"] = referenceFramebuffer->getSize();
		(*stochaisticDepthGradientProgram)["depthErrorScale"] = depthErrorScale.value;
		stochaisticDepthGradientProgram->dispatchCompute(referenceFramebuffer->getSize().x, referenceFramebuffer->getSize().y, 1);
	}
	else
	{
		(*stochaisticColorGradientProgram)["referenceImage"] = *referenceFramebuffer->getColorAttachments()[0];
		(*stochaisticColorGradientProgram)["screenSize"] = referenceFramebuffer->getSize();
		stochaisticColorGradientProgram->dispatchCompute(referenceFramebuffer->getSize().x, referenceFramebuffer->getSize().y, 1);
	}
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	return stochaisticGradientSSBO;
}

renderer::ssbo_ptr<float> visual::GradientCalculatorPos::getFloatParams()
{
	renderer::ssbo_ptr<float> floatData = 
		renderer::make_ssbo<float>(optimizedParamsSSBO->getSize() * ParticleShaderData::paramCount, GL_DYNAMIC_COPY);
	optimizedParamsSSBO->bindBuffer(0);
	floatData->bindBuffer(1);
	particleDataToFloatProgram->dispatchCompute(optimizedParamsSSBO->getSize() / 64 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	return floatData;
}

renderer::ssbo_ptr<visual::ParticleShaderData> visual::GradientCalculatorPos::getParticleData()
{
	return optimizedParamsSSBO;
}

size_t visual::GradientCalculatorPos::getOptimizedParamCountPerParticle() const
{
	return ParticleShaderData::paramCount;
}

