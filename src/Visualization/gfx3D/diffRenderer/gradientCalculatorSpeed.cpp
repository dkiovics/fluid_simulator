#include "gradientCalculatorSpeed.h"

visual::GradientCalculatorSpeed::GradientCalculatorSpeed(std::shared_ptr<ParamInterface> renderer, 
	std::shared_ptr<genericfsim::manager::SimulationManager> manager)
{
	renderer3D = renderer;
	this->manager = manager;

	perturbationProgram = renderer::make_compute("shaders/3D/diffRender/perturbation_speed.comp");
	stochaisticColorGradientProgram = renderer::make_compute("shaders/3D/diffRender/stochGradient_color_speed.comp");
	stochaisticDepthGradientProgram = renderer::make_compute("shaders/3D/diffRender/stochGradient_depth_speed.comp");

	(*stochaisticColorGradientProgram)["plusPertImage"] = *pertPlusFramebuffer->getColorAttachments()[0];
	(*stochaisticColorGradientProgram)["minusPertImage"] = *pertMinusFramebuffer->getColorAttachments()[0];
	(*stochaisticDepthGradientProgram)["plusPertImage"] = *pertPlusFramebuffer->getDepthAttachment();
	(*stochaisticDepthGradientProgram)["minusPertImage"] = *pertMinusFramebuffer->getDepthAttachment();

	auto camera = renderer3D->getCamera();
	camera->addProgram({ stochaisticDepthGradientProgram });
	camera->setUniformsForAllPrograms();

	addParamLine({ &speedPerturbation });
	addParamLine({ &simulationDt });
	addParamLine({ &simulationIterationCount });
	addParamLine({ &resetSpeedsAfterIteration, &speedCapEnabled, &simulatorEnabled, &gravityEnabled });
	addParamLine(ParamLine({ &speedCap }, &speedCapEnabled));
	addParamLine(ParamLine({ &gravityValue }, &gravityEnabled));
}

void visual::GradientCalculatorSpeed::updateOptimizedFloats(renderer::ssbo_ptr<float> data, renderer::ssbo_ptr<float> particleMovementAbs)
{
	if (data->getSize() != speedSSBO->getSize())
		throw std::runtime_error("GradientCalculatorSpeed::updateOptimizedFloats: data size does not match the speedSSBO size");
	data->copyTo(*speedSSBO);
	updateSimFromParticleData(optimizedParamsSSBO);
	simulateSpeeds(speedSSBO, true);
	updateOptimizedParticleDataFromSim(particleMovementAbs);
}

void visual::GradientCalculatorSpeed::updateParticleParams(renderer::ssbo_ptr<ParticleShaderData> data)
{
	GradientCalculatorInterface::updateParticleParams(data);
	if (!perturbationPresetSSBO || perturbationPresetSSBO->getSize() != data->getSize())
	{
		perturbationPresetSSBO = renderer::make_ssbo<ParticleShaderDataSpeed>(data->getSize(), GL_DYNAMIC_COPY);
		speedSSBO = renderer::make_ssbo<float>(data->getSize() * ParticleShaderDataSpeed::paramCount, GL_DYNAMIC_COPY);
		speedPositiveOffsetSSBO = renderer::make_ssbo<float>(data->getSize() * ParticleShaderDataSpeed::paramCount, GL_DYNAMIC_COPY);
		speedNegativeOffsetSSBO = renderer::make_ssbo<float>(data->getSize() * ParticleShaderDataSpeed::paramCount, GL_DYNAMIC_COPY);
	}
	data->copyTo(*optimizedParamsSSBO);
}

void visual::GradientCalculatorSpeed::reset()
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	perturbationPresetSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	unsigned int paramNum = perturbationPresetSSBO->getSize();
	for (unsigned int i = 0; i < paramNum; i++)
	{
		(*perturbationPresetSSBO)[i].speed = 
			glm::vec4(speedPerturbation.value, speedPerturbation.value, speedPerturbation.value, 0.0f);
	}
	perturbationPresetSSBO->unmapBuffer();
	simulatorCopy = manager->getSimulatorCopy();
	simulatorCopy.simulator->config.gravityEnabled = false;
	simulatorCopy.simulator->config.gravity = gravityValue.value;
	stochaisticGradientSSBO->fillWithZeros();
	gradientSampleCount = 0;
	speedSSBO->fillWithZeros();
}

bool visual::GradientCalculatorSpeed::calculateGradient(renderer::fb_ptr referenceFramebuffer)
{
	if (gradientSampleCount >= gradientSampleNum.value)
	{
		stochaisticGradientSSBO->fillWithZeros();
		gradientSampleCount = 0;
	}

	pertPlusFramebuffer->setSize(referenceFramebuffer->getSize());
	pertMinusFramebuffer->setSize(referenceFramebuffer->getSize());
	simulatorCopy.simulator->config.onlyMoveParticles = !simulatorEnabled.value;
	simulatorCopy.simulator->config.gravityEnabled = gravityEnabled.value;

	if (resetSpeedsAfterIteration.value)
	{
		speedSSBO->fillWithZeros();
	}

	(*perturbationProgram)["seed"] = std::rand() % 1000;
	speedSSBO->bindBuffer(0);
	perturbationPresetSSBO->bindBuffer(1);
	speedNegativeOffsetSSBO->bindBuffer(2);
	speedPositiveOffsetSSBO->bindBuffer(3);
	perturbationProgram->dispatchCompute(perturbationPresetSSBO->getSize() / 64 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	updateSimFromParticleData(optimizedParamsSSBO);
	simulateSpeeds(speedPositiveOffsetSSBO);
	updateParticleDataFromSim(paramPositiveOffsetSSBO);

	pertPlusFramebuffer->bind();
	renderEngine.clearViewport(glm::vec4(0.0f), 1.0f);
	renderer3D->render(pertPlusFramebuffer, paramPositiveOffsetSSBO);

	updateSimFromParticleData(optimizedParamsSSBO);
	simulateSpeeds(speedNegativeOffsetSSBO);
	updateParticleDataFromSim(paramNegativeOffsetSSBO);

	pertMinusFramebuffer->bind();
	renderEngine.clearViewport(glm::vec4(0.0f), 1.0f);
	renderer3D->render(pertMinusFramebuffer, paramNegativeOffsetSSBO);

	speedNegativeOffsetSSBO->bindBuffer(0);
	speedPositiveOffsetSSBO->bindBuffer(1);
	renderer3D->getParamBufferOut()->bindBuffer(2);
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
	gradientSampleCount++;
	bool gradientReady = gradientSampleCount >= gradientSampleNum.value;
	if (gradientReady)
		correctGradient();
	return gradientReady;
}

renderer::ssbo_ptr<float> visual::GradientCalculatorSpeed::getFloatParams()
{
	renderer::ssbo_ptr<float> floatData = 
		renderer::make_ssbo<float>(speedSSBO->getSize(), GL_DYNAMIC_COPY);
	speedSSBO->copyTo(*floatData);
	return floatData;
}

renderer::ssbo_ptr<visual::ParticleShaderData> visual::GradientCalculatorSpeed::getParticleData()
{
	return optimizedParamsSSBO;
}

size_t visual::GradientCalculatorSpeed::getOptimizedParamCountPerParticle() const
{
	return ParticleShaderDataSpeed::paramCount;
}

void visual::GradientCalculatorSpeed::formatFloatParamsPreUpdate(renderer::ssbo_ptr<float> data) const
{
	if (resetSpeedsAfterIteration.value)
	{
		data->fillWithZeros();
	}
	else
	{
		speedSSBO->copyTo(*data);
	}
}

void visual::GradientCalculatorSpeed::formatFloatParamsPostUpdate(renderer::ssbo_ptr<float> data) const
{
	if (speedCapEnabled.value)
	{
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		data->mapBuffer(0, -1, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
		for (int i = 0; i < data->getSize(); i += ParticleShaderDataSpeed::paramCount)
		{
			glm::vec3 speed((*data)[i], (*data)[i + 1], (*data)[i + 2]);
			if (glm::length(speed) > speedCap.value)
			{
				speed = glm::normalize(speed) * speedCap.value;
				(*data)[i] = speed.x;
				(*data)[i + 1] = speed.y;
				(*data)[i + 2] = speed.z;
			}
		}
		data->unmapBuffer();
	}
}

void visual::GradientCalculatorSpeed::simulateSpeeds(renderer::ssbo_ptr<float> speeds, bool updateSpeeds)
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	simulatorCopy.hashedParticles->setParticleNum(speeds->getSize() / ParticleShaderDataSpeed::paramCount);
	if(updateSpeeds)
		speeds->mapBuffer(0, -1, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
	else
		speeds->mapBuffer(0, -1, GL_MAP_READ_BIT);
	simulatorCopy.hashedParticles->forEach(true, [&](auto& particle, int index) {
		particle.v = glm::vec3((*speeds)[index * ParticleShaderDataSpeed::paramCount],
			(*speeds)[index * ParticleShaderDataSpeed::paramCount + 1],
			(*speeds)[index * ParticleShaderDataSpeed::paramCount + 2]);
	});
	for (int i = 0; i < simulationIterationCount.value; i++)
	{
		simulatorCopy.simulator->simulate(simulationDt.value);
	}
	if (updateSpeeds)
	{
		simulatorCopy.hashedParticles->forEach(true, [&](auto& particle, int index) {
			(*speeds)[index * ParticleShaderDataSpeed::paramCount] = particle.v.x;
			(*speeds)[index * ParticleShaderDataSpeed::paramCount + 1] = particle.v.y;
			(*speeds)[index * ParticleShaderDataSpeed::paramCount + 2] = particle.v.z;
			});
	}
	speeds->unmapBuffer();
}

void visual::GradientCalculatorSpeed::updateParticleDataFromSim(renderer::ssbo_ptr<ParticleShaderData> data)
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	data->mapBuffer(0, -1, GL_MAP_WRITE_BIT);
	simulatorCopy.hashedParticles->forEach(true, [&](auto& particle, int index) {
		(*data)[index].posAndSpeed = glm::vec4(particle.pos, glm::length(particle.v));
	});
	data->unmapBuffer();
}

void visual::GradientCalculatorSpeed::updateOptimizedParticleDataFromSim(renderer::ssbo_ptr<float> particleMovementAbs)
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	optimizedParamsSSBO->mapBuffer(0, -1, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
	particleMovementAbs->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	simulatorCopy.hashedParticles->forEach(true, [&](auto& particle, int index) {
		(*particleMovementAbs)[index] = glm::length(particle.pos - glm::dvec3((*optimizedParamsSSBO)[index].posAndSpeed));
		(*optimizedParamsSSBO)[index].posAndSpeed = glm::vec4(particle.pos, glm::length(particle.v));
	});
	particleMovementAbs->unmapBuffer();
	optimizedParamsSSBO->unmapBuffer();
}

void visual::GradientCalculatorSpeed::updateSimFromParticleData(renderer::ssbo_ptr<ParticleShaderData> data)
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	data->mapBuffer(0, -1, GL_MAP_READ_BIT);
	simulatorCopy.hashedParticles->forEach(true, [&](auto& particle, int index) {
		particle.pos = glm::vec3((*data)[index].posAndSpeed);
		particle.c[0] = particle.c[1] = particle.c[2] = glm::dvec3(0, 0, 0);
	});
	data->unmapBuffer();
}
