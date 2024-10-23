#include "diffRenderProxy.h"
#include <iostream>
#include "gradientCalculatorPos.h"
#include "gradientCalculatorSpeed.h"

using namespace visual;

DiffRendererProxy::DiffRendererProxy(std::shared_ptr<Renderer3DInterface> renderer3D)
	:renderer3D(std::static_pointer_cast<SimulationGfx3DRenderer>(renderer3D)), renderEngine(renderer::RenderEngine::getInstance())
{
	referenceFramebuffer = renderer::make_fb(
		renderer::Framebuffer::toArray({ 
			renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE),
		}),
		renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT),
		false
	);

	particleMovementAbsSSBO = renderer::make_ssbo<float>(1000, GL_DYNAMIC_COPY);
	particleGradientSSBO = renderer::make_ssbo<GradientCalculatorInterface::ParticleGradientData>(1000, GL_DYNAMIC_COPY);

	showQuad = std::make_unique<renderer::Square>();
	showProgram = renderer::make_shader("shaders/3D/util/quad.vert", "shaders/3D/util/quad.frag");
	
	gradientArrowShader = renderer::make_shader("shaders/3D/util/arrow.vert", "shaders/3D/util/arrow.frag");
	gradientArrows = std::make_unique<renderer::InstancedGeometry>(std::make_shared<renderer::Arrow4>(0.1f, 1.2f, 0.6f));

	adam = std::make_unique<AdamOptimizer>(1);
	densityControl = std::make_unique<DensityControl>();

	addParamLine({ &updateReference, &updateParams, &updateSimulatorButton, &resetAdamButton,  &randomizeParams, &doSimulatorGradientCalc });
	addParamLine({ &showReference, &showSim, &adamEnabled, &updateDensities, &enableDensityControl });
	addParamLine({ &autoPushApart, &pushApartButton, &backupCameraPos, &restoreCameraPos, &gradientVisualization });
	addParamLine(ParamLine({ &pushApartUpdatePeriod }, &autoPushApart ));
	addParamLine(ParamLine({ &arrowDensityThreshold }, &gradientVisualization));

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
}

void DiffRendererProxy::render(renderer::fb_ptr framebuffer, renderer::ssbo_ptr<ParticleShaderData> data)
{
	renderEngine.setViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);

	if(updateReference.value)
	{
		renderer3D->render(referenceFramebuffer, data);
		return;
	}

	bool gradientCalcChanged = false;
	if (doSimulatorGradientCalc.value)
	{
		if (!gradientCalculator || !dynamic_cast<GradientCalculatorSpeed*>(gradientCalculator.get()))
		{
			gradientCalculator = std::make_unique<GradientCalculatorSpeed>(renderer3D, configData.simManager);
			gradientCalcChanged = true;
		}
	}
	else
	{
		if (!gradientCalculator || !dynamic_cast<GradientCalculatorPos*>(gradientCalculator.get()))
		{
			gradientCalculator = std::make_unique<GradientCalculatorPos>(renderer3D, configData.simManager);
			gradientCalcChanged = true;
		}
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

	if (backupCameraPos.value)
	{
		backupCamera = renderer3D->getCamera()->getCameraData();
	}
	if (restoreCameraPos.value && backupCamera)
	{
		renderer3D->getCamera()->setCameraData(*backupCamera);
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
		auto optimizedParamsSSBO = gradientCalculator->getParticleData();
		if(adamEnabled.value)
		{
			if (newFluidParamsNeeded)
			{
				renderer3D->invalidateParamBuffer();
				newFluidParamsNeeded = false;
			}
			renderer3D->renderBoxFrontEnabled = false;
			renderer3D->render(framebuffer, optimizedParamsSSBO);
			renderer3D->renderBoxFrontEnabled = true;

			gradientCalculator->formatFloatParamsPreUpdate(adam->getOptimizedFloatData());
			if (gradientCalculator->calculateGradient(referenceFramebuffer))
			{
				newFluidParamsNeeded = true;
				adam->optimize(gradientCalculator->getStochaisticGradient());

				gradientCalculator->formatFloatParamsPostUpdate(adam->getOptimizedFloatData());
				gradientCalculator->updateOptimizedFloats(adam->getOptimizedFloatData(), particleMovementAbsSSBO);

				if (gradientVisualization.value)
				{
					particleGradientValid = true;
					gradientCalculator->getParticleGradient(particleGradientSSBO);
				}

				if (updateDensities.value)
				{
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
		else
		{
			renderer3D->renderBoxFrontEnabled = false;
			renderer3D->render(framebuffer, optimizedParamsSSBO);
			renderer3D->renderBoxFrontEnabled = true;
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
	}
}

void DiffRendererProxy::setConfigData(const ConfigData3D& data)
{
	renderer3D->setConfigData(data);
	configData = data;
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

