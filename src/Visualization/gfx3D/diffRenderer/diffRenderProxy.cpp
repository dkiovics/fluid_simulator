#include "diffRenderProxy.h"
#include <iostream>
#include "gradientCalculatorPos.h"
#include "gradientCalculatorSpeed.h"

using namespace visual;

DiffRendererProxy::DiffRendererProxy(std::shared_ptr<Renderer3DInterface> renderer3D)
	:renderer3D(std::static_pointer_cast<SimulationGfx3DRenderer>(renderer3D)), renderEngine(renderer::WindowManager::getInstance())
{
	for (auto& referenceData : referenceDataArray)
	{
		referenceData.colorRenderTargetTexture = renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
		referenceData.depthRenderTargetTexture = renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
	}

	referenceFramebuffer = renderer::make_fb(
		renderer::Framebuffer::toArray({
			referenceDataArray[0].colorRenderTargetTexture
		}),
		referenceDataArray[0].depthRenderTargetTexture,
		false
	);

	particleMovementAbsSSBO = renderer::make_ssbo<float>(1000, GL_DYNAMIC_COPY);
	particleGradientSSBO = renderer::make_ssbo<GradientCalculatorInterface::ParticleGradientData>(1000, GL_DYNAMIC_COPY);
	errorValueSSBO = renderer::make_ssbo<float>(1, GL_DYNAMIC_COPY);

	showQuad = std::make_unique<renderer::Square>();
	showProgram = renderer::make_shader("shaders/3D/util/quad.vert", "shaders/3D/util/quad.frag");
	errorCompute = renderer::make_compute("shaders/3D/diffRender/errorCalculation.comp");
	
	gradientArrowShader = renderer::make_shader("shaders/3D/util/arrow.vert", "shaders/3D/util/arrow.frag");
	gradientArrows = std::make_unique<renderer::InstancedGeometry>(std::make_shared<renderer::Arrow4>(0.1f, 1.2f, 0.6f));

	adam = std::make_unique<AdamOptimizer>(1);
	densityControl = std::make_unique<DensityControl>();

	addParamLine({ &updateReference, &updateParams, &updateSimulatorButton, &resetAdamButton,  &randomizeParams, &doSimulatorGradientCalc });
	addParamLine({ &showReference, &showSim, &adamEnabled, &updateDensities, &enableDensityControl });
	addParamLine({ &referenceImageCount, &referenceImageIndex });
	addParamLine({ &autoPushApart, &pushApartButton, &restoreCameraPos, &gradientVisualization, &enableGradientSmoothing });
	addParamLine({ &referenceImageFileName, &loadReferenceImageButton, &storeReferenceImageButton, &printErrorValue });
	addParamLine(ParamLine({ &pushApartUpdatePeriod }, &autoPushApart ));
	addParamLine(ParamLine({ &arrowDensityThreshold }, &gradientVisualization));
	addParamLine(ParamLine({ &gradientSmoothingSphereR }, &enableGradientSmoothing));

	auto camera = this->renderer3D->getCamera();
	auto lights = this->renderer3D->getLights();
	if (camera)
	{
		camera->addProgram({ gradientArrowShader });
		camera->setUniformsForAllPrograms();
	}
	if (lights)
	{
		lights->addProgram({ gradientArrowShader });
		lights->setUniformsForAllPrograms();
	}

	this->renderer3D->setParamBufferOutCnt(1);
	handleReferenceImageCountChange();
}

void DiffRendererProxy::render(renderer::fb_ptr framebuffer, renderer::ssbo_ptr<ParticleShaderData> data)
{
	renderEngine.setViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);

	if(updateReference.value)
	{
		referenceDataArray[currentReferenceIndex].backupCamera = renderer3D->getCamera()->getCameraData();
		renderer3D->render(referenceFramebuffer, data); 
		return;
	}

	if (loadReferenceImageButton.value)
	{
		if (!referenceFramebuffer->getColorAttachments()[0]->loadImage(referenceImageFileName.getValue()))
		{
			spdlog::warn("Failed to load image: {}", referenceImageFileName.getValue());
		}
	}

	if (storeReferenceImageButton.value)
	{
		referenceFramebuffer->getColorAttachments()[0]->storeImage(referenceImageFileName.getValue());
	}

	bool gradientCalcChanged = false;
	if (doSimulatorGradientCalc.value)
	{
		if (!gradientCalculator || !dynamic_cast<GradientCalculatorSpeed*>(gradientCalculator.get()))
		{
			gradientCalculator = std::make_unique<GradientCalculatorSpeed>(renderer3D, configData.simManager, maxReferenceImageCount);
			gradientCalcChanged = true;
		}
	}
	else
	{
		if (!gradientCalculator || !dynamic_cast<GradientCalculatorPos*>(gradientCalculator.get()))
		{
			gradientCalculator = std::make_unique<GradientCalculatorPos>(renderer3D, configData.simManager, maxReferenceImageCount);
			gradientCalcChanged = true;
		}
	}

	if (!gradientSmoothing 
		|| gradientSmoothing->particleBox != glm::vec3(configData.simManager->getDimensions())
		|| gradientSmoothingSphereR.value != gradientSmoothing->smoothingSphereR)
	{
		gradientSmoothing = std::make_unique<GradientSmoothing>
			(gradientSmoothingSphereR.value, configData.simManager->getDimensions());
	}

	if(!particleMovementAbsSSBO || data->getSize() != particleMovementAbsSSBO->getSize() || gradientCalcChanged)
	{
		particleMovementAbsSSBO->setSize(data->getSize());
		particleGradientSSBO->setSize(data->getSize());
		adam->setParamNum(data->getSize() * gradientCalculator->getOptimizedParamCountPerParticle());
		densityControl->setParamNum(data->getSize());
		reset(data);
		particleGradientValid = false;
	}

	if (restoreCameraPos.value && referenceDataArray[currentReferenceIndex].backupCamera)
	{
		renderer3D->getCamera()->setCameraData(*referenceDataArray[currentReferenceIndex].backupCamera);
	}

	if (updateSimulatorButton.value)
	{
		updateSimulator();
	}

	if (pushApartButton.value)
	{
		pushApartOptimizedParams();
	}

	if(randomizeParams.value)
	{
		randomizeParamValues(data);
		particleGradientValid = false;
	}

	if(resetAdamButton.value)
	{
		adam->set(gradientCalculator->getFloatParams());
		adam->reset();
		densityControl->reset();
	}

	if(updateParams.value)
	{
		reset(data);
		particleGradientValid = false;
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
		bool adamStepHappened = false;
		auto optimizedParamsSSBO = gradientCalculator->getParticleData();
		if (adamEnabled.value && !validateState())
		{
			adamEnabled.value = false;
		}
		if(adamEnabled.value)
		{
			if (newFluidParamsNeeded)
			{
				renderer3D->invalidateParamBuffer();
				newFluidParamsNeeded = false;
				for (size_t i = 0; i < referenceImageCountValue; i++)
				{
					if (i != currentReferenceIndex)
					{
						renderer3D->getCamera()->setCameraData(*referenceDataArray[i].backupCamera);
						renderer3D->setActiveParamBuffer(int(i));
						renderer3D->render(framebuffer, optimizedParamsSSBO);
					}
				}
				renderer3D->getCamera()->setCameraData(*referenceDataArray[currentReferenceIndex].backupCamera);
				renderer3D->setActiveParamBuffer(currentReferenceIndex);
				renderer3D->renderBoxFrontEnabled = false;
				renderer3D->render(framebuffer, optimizedParamsSSBO);
			}
			else
			{
				renderer3D->getCamera()->setCameraData(*referenceDataArray[currentReferenceIndex].backupCamera);
				renderer3D->renderBoxFrontEnabled = false;
				renderer3D->render(framebuffer, optimizedParamsSSBO);
				std::vector<ReferenceData> referenceDataVector(referenceDataArray.begin(), referenceDataArray.begin() + referenceImageCountValue);
				renderer3D->renderBoxFrontEnabled = true;
				if (gradientCalculator->calculateGradient(referenceDataVector))
				{
					adamStepHappened = true;
					newFluidParamsNeeded = true;

					gradientCalculator->getParticleGradient(particleGradientSSBO);
					particleGradientValid = true;

					if (enableGradientSmoothing.value)
					{
						gradientSmoothing->smoothGradient(particleGradientSSBO);
						gradientCalculator->setParticleGradient(particleGradientSSBO);
					}

					gradientCalculator->formatFloatParamsPreUpdate(adam->getOptimizedFloatData());
					adam->optimize(gradientCalculator->getStochaisticGradient());
					gradientCalculator->formatFloatParamsPostUpdate(adam->getOptimizedFloatData());

					gradientCalculator->updateOptimizedFloats(adam->getOptimizedFloatData(), particleMovementAbsSSBO);

					if (updateDensities.value || enableDensityControl.value)
					{
						updateDensities.value = true;
						updateParticleDensities();
					}

					if (enableDensityControl.value)
					{
						if (densityControl->updateAvgMovement(particleMovementAbsSSBO))
						{
							densityControl->updatePositions(optimizedParamsSSBO);
							updateOptimizedParams(optimizedParamsSSBO);
						}
					}

					static int pushApartCounter = 0;
					if (autoPushApart.value)
					{
						pushApartCounter++;
						if (pushApartCounter >= pushApartUpdatePeriod.value)
						{
							pushApartOptimizedParams();
							pushApartCounter = 0;
						}
					}
					else
					{
						pushApartCounter = 0;
					}
				}
			}
			renderer3D->renderBoxFrontEnabled = true;
			renderer3D->getCamera()->setCameraData(*referenceDataArray[currentReferenceIndex].backupCamera);
		}
		else
		{
			renderer3D->renderBoxFrontEnabled = false;
			renderer3D->render(framebuffer, optimizedParamsSSBO);
			renderer3D->renderBoxFrontEnabled = true;
			newFluidParamsNeeded = true;
		}
		if (gradientVisualization.value && particleGradientValid)
		{
			gradientArrows->setInstanceNum(particleGradientSSBO->getSize());
			particleGradientSSBO->bindBuffer(8);
			gradientArrowShader->activate();
			(*gradientArrowShader)["densityThreshold"] = arrowDensityThreshold.value;
			framebuffer->bind();
			gradientArrows->draw();
		}
		renderer3D->showBoxFront(framebuffer);

		if (adamStepHappened && printErrorValue.value)
		{
			errorValueSSBO->setSize(framebuffer->getSize().y);
			errorValueSSBO->fillWithZeros();
			errorValueSSBO->bindBuffer(0);
			(*errorCompute)["referenceImage"] = *referenceFramebuffer->getColorAttachments()[0];
			(*errorCompute)["currentImage"] = *framebuffer->getColorAttachments()[0];
			(*errorCompute)["width"] = framebuffer->getSize().x;
			errorCompute->dispatchCompute(framebuffer->getSize().y, 1, 1);
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			errorValueSSBO->mapBuffer(0, -1, GL_MAP_READ_BIT);
			float sum = 0.0f;
			for (uint32_t i = 0; i < errorValueSSBO->getSize(); i++)
			{
				sum += (*errorValueSSBO)[i];
			}
			errorValueSSBO->unmapBuffer();
			std::cout << sum << std::endl;
		}
	}
}

void DiffRendererProxy::setConfigData(const ConfigData3D& data)
{
	renderer3D->setConfigData(data);
	renderer3D->setParamBuffersRes(data.screenSize);
	configData = data;
	for (auto& referenceData : referenceDataArray)
	{
		referenceData.colorRenderTargetTexture->resizeTexture(data.screenSize.x, data.screenSize.y);
		referenceData.depthRenderTargetTexture->resizeTexture(data.screenSize.x, data.screenSize.y);
	}
	referenceFramebuffer->setSize(data.screenSize);
}

void DiffRendererProxy::show(int screenWidth)
{
	ImGui::SeparatorText("DiffRendererProxy");
	ParamLineCollection::show(screenWidth);
	if(gradientCalculator)
		gradientCalculator->show(screenWidth);
	renderer3D->show(screenWidth);
	ImGui::Begin("Adam");
	adam->show(screenWidth * 2);
	ImGui::End();
	if (enableDensityControl.value)
	{
		ImGui::Begin("Density control");
		densityControl->show(screenWidth);
		ImGui::End();
	}

	handleReferenceImageCountChange();
}

void visual::DiffRendererProxy::reset(renderer::ssbo_ptr<ParticleShaderData> data)
{
	gradientCalculator->updateParticleParams(data);
	gradientCalculator->reset();
	adam->set(gradientCalculator->getFloatParams());
	adam->reset();
	densityControl->reset();
}

void visual::DiffRendererProxy::randomizeParamValues(renderer::ssbo_ptr<ParticleShaderData> baselineData)
{
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
	auto optimizedParamsSSBO = gradientCalculator->getParticleData();
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
	reset(optimizedParamsSSBO);
}

void visual::DiffRendererProxy::updateOptimizedParams(renderer::ssbo_ptr<ParticleShaderData> data)
{
	gradientCalculator->updateParticleParams(data);
	adam->set(gradientCalculator->getFloatParams());
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

void visual::DiffRendererProxy::pushApartOptimizedParams()
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	auto optimizedParamsSSBO = gradientCalculator->getParticleData();
	auto hashedParticles = configData.simManager->getHashedParticlesCopy();
	hashedParticles->setParticleNum(optimizedParamsSSBO->getSize());
	optimizedParamsSSBO->mapBuffer(0, -1, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
	hashedParticles->forEach(true, [&](auto& particle, int index) {
		particle.pos = glm::vec3((*optimizedParamsSSBO)[index].posAndSpeed);
	});
	hashedParticles->updateParticleIntersectionHash(true);
	hashedParticles->pushParticlesApart(true);
	hashedParticles->forEach(true, [&](auto& particle, int index) {
		(*optimizedParamsSSBO)[index].posAndSpeed = glm::vec4(particle.pos, (*optimizedParamsSSBO)[index].posAndSpeed.w);
	});
	optimizedParamsSSBO->unmapBuffer();
	updateOptimizedParams(optimizedParamsSSBO);
}

void visual::DiffRendererProxy::updateParticleDensities()
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	auto hashedParticles = configData.simManager->getHashedParticlesCopy();
	auto optimizedParamsSSBO = gradientCalculator->getParticleData();
	hashedParticles->setParticleNum(optimizedParamsSSBO->getSize());
	optimizedParamsSSBO->mapBuffer(0, -1, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
	hashedParticles->forEach(true, [&](auto& particle, int index) {
		particle.pos = glm::vec3((*optimizedParamsSSBO)[index].posAndSpeed);
	});
	auto densities = configData.simManager->calculateParticleDensity(hashedParticles);
	for (int i = 0; i < densities.size(); i++)
	{
		(*optimizedParamsSSBO)[i].density = glm::vec4(densities[i]);
	}
	optimizedParamsSSBO->unmapBuffer();
	updateOptimizedParams(optimizedParamsSSBO);
}

void visual::DiffRendererProxy::updateSimulator()
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	auto optimizedParamsSSBO = gradientCalculator->getParticleData();
	optimizedParamsSSBO->mapBuffer(0, -1, GL_MAP_READ_BIT);
	auto particles = configData.simManager->getHashedParticlesCopy();
	particles->setParticleNum(optimizedParamsSSBO->getSize());
	const double r = particles->getParticleR();
	const glm::dvec3 cellD = configData.simManager->getCellD();
	const glm::dvec3 lowerLimit = cellD + r;
	const glm::dvec3 upperLimit = configData.simManager->getDimensions() - cellD - r;
	particles->forEach(true, [&](auto& particle, int index) {
		particle.pos = glm::vec3((*optimizedParamsSSBO)[index].posAndSpeed);
		particle.pos = glm::clamp(particle.pos, lowerLimit, upperLimit);
		particle.v = glm::dvec3(0.0);
		particle.c[0] = particle.c[1] = particle.c[2] = glm::dvec3(0.0);
	});
	optimizedParamsSSBO->unmapBuffer();
	particles->updateParticleIntersectionHash(true);
	configData.simManager->setHashedParticles(std::move(particles));
}

void visual::DiffRendererProxy::handleReferenceImageCountChange()
{
	if (referenceImageCountValue != referenceImageCount.value)
	{
		referenceImageCountValue = referenceImageCount.value;
		renderer3D->setParamBufferOutCnt(referenceImageCountValue);
		referenceImageIndex.options.clear();
		for (int i = 0; i < referenceImageCountValue; i++)
		{
			referenceImageIndex.options.push_back(std::to_string(i + 1));
		}
		referenceImageIndex.value = 0;
	}
	if (currentReferenceIndex != referenceImageIndex.value)
	{
		currentReferenceIndex = referenceImageIndex.value;
		referenceFramebuffer->setColorAttachments(renderer::Framebuffer::toArray({
			referenceDataArray[currentReferenceIndex].colorRenderTargetTexture
		}));
		referenceFramebuffer->setDepthAttachment(referenceDataArray[currentReferenceIndex].depthRenderTargetTexture);
		if (referenceDataArray[currentReferenceIndex].backupCamera)
		{
			renderer3D->getCamera()->setCameraData(*referenceDataArray[currentReferenceIndex].backupCamera);
		}
	}
}

bool visual::DiffRendererProxy::validateState() const
{
	for (int p = 0; p < referenceImageCountValue; p++)
	{
		if (!referenceDataArray[p].backupCamera)
		{
			spdlog::warn("No camera data for reference image {}", p);
			return false;
		}
	}
	return true;
}

