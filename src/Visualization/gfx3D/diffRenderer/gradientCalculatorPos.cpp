#include "gradientCalculatorPos.h"

visual::GradientCalculatorPos::GradientCalculatorPos(std::shared_ptr<ParamInterface> renderer, 
	std::shared_ptr<genericfsim::manager::SimulationManager> manager, size_t maxReferenceCount)
	: GradientCalculatorInterface(maxReferenceCount)
{
	renderer3D = renderer;
	this->manager = manager;

	perturbationProgram = renderer::make_compute("shaders/3D/diffRender/perturbation.comp");
	stochaisticColorGradientProgram = renderer::make_compute("shaders/3D/diffRender/stochGradient_color.comp");
	stochaisticDepthGradientProgram = renderer::make_compute("shaders/3D/diffRender/stochGradient_depth.comp");
	particleDataToFloatProgram = renderer::make_compute("shaders/3D/diffRender/particleDataToFloat.comp");
	floatToParticleDataProgram = renderer::make_compute("shaders/3D/diffRender/floatToParticleData.comp");
	floatPosClamperProgram = renderer::make_compute("shaders/3D/diffRender/floatPosClamper.comp");

	std::vector<std::shared_ptr<renderer::Texture>> plusColor;
	std::vector<std::shared_ptr<renderer::Texture>> minusColor;
	std::vector<std::shared_ptr<renderer::Texture>> plusDepth;
	std::vector<std::shared_ptr<renderer::Texture>> minusDepth;
	for (size_t i = 0; i < maxReferenceCount; i++)
	{
		plusColor.push_back(perturbedRenderedScenes[i].first.color);
		minusColor.push_back(perturbedRenderedScenes[i].second.color);
		plusDepth.push_back(perturbedRenderedScenes[i].first.depth);
		minusDepth.push_back(perturbedRenderedScenes[i].second.depth);
	}

	(*stochaisticColorGradientProgram)["plusPertImage"] = plusColor;
	(*stochaisticColorGradientProgram)["minusPertImage"] = minusColor;
	(*stochaisticDepthGradientProgram)["plusPertImage"] = plusDepth;
	(*stochaisticDepthGradientProgram)["minusPertImage"] = minusDepth;

	auto camera = renderer3D->getCamera();
	camera->addProgram({ stochaisticDepthGradientProgram });
	camera->setUniformsForAllPrograms();

	addParamLine({ &speedAbsPerturbation });
	addParamLine({ &posPerturbation });
	addParamLine({ &capPositionsToBox });
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
	GradientCalculatorInterface::updateParticleParams(data);
	if (!perturbationPresetSSBO || perturbationPresetSSBO->getSize() != data->getSize())
	{
		perturbationPresetSSBO = renderer::make_ssbo<ParticleShaderData>(data->getSize(), GL_DYNAMIC_COPY);
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
	stochaisticGradientSSBO->fillWithZeros();
	gradientSampleCount = 0;
}

bool visual::GradientCalculatorPos::calculateGradient(std::vector<ReferenceData> referenceData)
{
	if (gradientSampleCount >= gradientSampleNum.value)
	{
		stochaisticGradientSSBO->fillWithZeros();
		gradientSampleCount = 0;
	}

	setRenderedSceneSizes(referenceData[0].colorRenderTargetTexture->getSize().x, referenceData[0].colorRenderTargetTexture->getSize().y);

	if (referenceData.size() > perturbedRenderedScenes.size())
		throw std::runtime_error("GradientCalculatorSpeed::calculateGradient: referenceData size exceeds maxReferenceCount");

	setRenderedSceneSizes(referenceData[0].colorRenderTargetTexture->getSize().x, referenceData[0].colorRenderTargetTexture->getSize().y);
	pertPlusFramebuffer->setSize(referenceData[0].colorRenderTargetTexture->getSize());
	pertMinusFramebuffer->setSize(referenceData[0].colorRenderTargetTexture->getSize());

	(*perturbationProgram)["seed"] = std::rand() % 1000;
	auto boxBounds = getBoxBounds();
	(*perturbationProgram)["boxLowerBound"] = boxBounds.first; 
	(*perturbationProgram)["boxUpperBound"] = boxBounds.second;
	(*perturbationProgram)["posClampEnabled"] = capPositionsToBox.value;
	optimizedParamsSSBO->bindBuffer(0);
	perturbationPresetSSBO->bindBuffer(1);
	paramNegativeOffsetSSBO->bindBuffer(2);
	paramPositiveOffsetSSBO->bindBuffer(3);
	perturbationProgram->dispatchCompute(optimizedParamsSSBO->getSize() / 64 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	
	for (size_t i = 0; i < referenceData.size(); i++)
	{
		bindPerturbedRenderedSceneTextures(i);

		if (referenceData[i].backupCamera)
			renderer3D->getCamera()->setCameraData(referenceData[i].backupCamera.value());
		else
			spdlog::warn("GradientCalculatorSpeed::calculateGradient: backup camera is not set");

		renderer3D->render(pertPlusFramebuffer, paramPositiveOffsetSSBO);
		renderer3D->render(pertMinusFramebuffer, paramNegativeOffsetSSBO);
	}

	paramNegativeOffsetSSBO->bindBuffer(0);
	paramPositiveOffsetSSBO->bindBuffer(1);
	stochaisticGradientSSBO->bindBuffer(2);
	for (int p = 0; p < referenceData.size(); p++)
	{
		renderer3D->getParamBufferOut(p).first->bindBuffer(3 + p);
	}
	if (useDepthImage.value)
	{
		std::vector<std::shared_ptr<renderer::Texture>> depthImages;
		for (size_t i = 0; i < referenceData.size(); i++)
		{
			depthImages.push_back(referenceData[i].depthRenderTargetTexture);
		}
		(*stochaisticDepthGradientProgram)["referenceImage"] = depthImages;
		(*stochaisticDepthGradientProgram)["referenceImageNum"] = (int)referenceData.size();
		(*stochaisticDepthGradientProgram)["screenSize"] = referenceData[0].colorRenderTargetTexture->getSize();
		(*stochaisticDepthGradientProgram)["depthErrorScale"] = depthErrorScale.value;
		stochaisticDepthGradientProgram->dispatchCompute(referenceData[0].colorRenderTargetTexture->getSize().x, referenceData[0].colorRenderTargetTexture->getSize().y, 1);
	}
	else
	{
		std::vector<std::shared_ptr<renderer::Texture>> colorImages;
		for (size_t i = 0; i < referenceData.size(); i++)
		{
			colorImages.push_back(referenceData[i].colorRenderTargetTexture);
		}
		(*stochaisticColorGradientProgram)["referenceImage"] = colorImages;
		(*stochaisticColorGradientProgram)["referenceImageNum"] = (int)referenceData.size();
		(*stochaisticColorGradientProgram)["screenSize"] = referenceData[0].colorRenderTargetTexture->getSize();
		stochaisticColorGradientProgram->dispatchCompute(referenceData[0].colorRenderTargetTexture->getSize().x, referenceData[0].colorRenderTargetTexture->getSize().y, 1);
	}
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	gradientSampleCount++;
	bool gradientReady = gradientSampleCount >= gradientSampleNum.value;
	if(gradientReady)
		correctGradient();
	return gradientReady;
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

void visual::GradientCalculatorPos::formatFloatParamsPostUpdate(renderer::ssbo_ptr<float> data) const
{
	if(!capPositionsToBox.value)
		return;
	data->bindBuffer(0);
	auto boxBounds = getBoxBounds();
	(*floatPosClamperProgram)["boxLowerBound"] = boxBounds.first;
	(*floatPosClamperProgram)["boxUpperBound"] = boxBounds.second;
	floatPosClamperProgram->dispatchCompute(optimizedParamsSSBO->getSize() / 64 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

std::pair<glm::vec3, glm::vec3> visual::GradientCalculatorPos::getBoxBounds() const
{
	const glm::vec3 cellD = manager->getCellD();
	const float particleRadius = manager->getConfig().particleRadius;
	return std::make_pair(cellD + particleRadius, glm::vec3(manager->getDimensions()) - cellD - particleRadius);
}

