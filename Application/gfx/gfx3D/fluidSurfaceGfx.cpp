#include "fluidSurfaceGfx.h"

FluidSurfaceGfx::FluidSurfaceGfx(std::shared_ptr<renderer::RenderEngine> engine,
	std::shared_ptr<renderer::Framebuffer> renderTargetFramebuffer, std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager,
	std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights, unsigned int maxParticleNum)
	: engine(engine), simulationManager(simulationManager), camera(camera), lights(lights), renderTargetFramebuffer(renderTargetFramebuffer)
{
	particleSpritesDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/particle_sprites.vs", "shaders/particle_sprites_depth.fs", engine);
	gaussianBlurShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/gaussian.fs", engine);
	shadedDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/shadedDepth.fs", engine);
	bilateralFilterShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/bilateral.fs", engine);
	fluidThicknessShader = std::make_shared<renderer::ShaderProgram>("shaders/particle_sprites.vs", "shaders/particle_sprites_thickness.fs", engine);
	fluidThicknessBlurShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/gaussian_thickness.fs", engine);
	normalAndDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/normal_depth.fs", engine);

	surfaceSquareArrayObject = std::make_unique<renderer::Object3D<renderer::ParticleGeometryArray>>
		(std::make_shared<renderer::ParticleGeometryArray>(std::make_shared<renderer::FlipSquare>()), particleSpritesDepthShader);
	surfaceSquareArrayObject->drawable->setMaxInstanceNum(maxParticleNum);

	spraySquareArrayObject = std::make_unique<renderer::Object3D<renderer::ParticleGeometryArray>>
		(std::make_shared<renderer::ParticleGeometryArray>(std::make_shared<renderer::FlipSquare>()), particleSpritesDepthShader);
	spraySquareArrayObject->drawable->setMaxInstanceNum(maxParticleNum);

	shadedSquareObject = std::make_unique<renderer::Object3D<renderer::Square>>(std::make_shared<renderer::Square>(), shadedDepthShader);
	shadedSquareObject->shininess = 80;
	shadedSquareObject->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);

	square = std::make_unique<renderer::Square>();

	{
		std::shared_ptr<renderer::RenderTargetTexture> depthTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
		depthFramebuffer = std::make_unique<renderer::Framebuffer>
			(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>(), depthTexture, false);
		std::shared_ptr<renderer::RenderTargetTexture> depthBlurTmpTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
		depthBlurTmpFramebuffer = std::make_unique<renderer::Framebuffer>
			(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>(), depthBlurTmpTexture, false);
		std::shared_ptr<renderer::RenderTargetTexture> normalAndDepthTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		normalAndDepthFramebuffer = std::make_unique<renderer::Framebuffer>
			(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>{ normalAndDepthTexture }, nullptr, false);
	}

	{
		std::shared_ptr<renderer::RenderTargetTexture> fluidThicknessTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);
		fluidThicknessFramebuffer = std::make_unique<renderer::Framebuffer>
			(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>{ fluidThicknessTexture },
				renderTargetFramebuffer->getDepthAttachment(), false);

		std::shared_ptr<renderer::RenderTargetTexture> fluidThicknessBlurTmpTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);
		fluidThicknessBlurTmpFramebuffer = std::make_unique<renderer::Framebuffer>
			(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>{ fluidThicknessBlurTmpTexture }, nullptr, false);
	}

	(*shadedDepthShader)["normalAndDepthTexture"] = *normalAndDepthFramebuffer->getColorAttachments()[0];
	(*shadedDepthShader)["thicknessTexture"] = *fluidThicknessFramebuffer->getColorAttachments()[0];
	(*fluidThicknessShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
	(*normalAndDepthShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();

	camera->addProgram({ particleSpritesDepthShader, gaussianBlurShader, 
		bilateralFilterShader, shadedDepthShader, fluidThicknessShader, fluidThicknessBlurShader, normalAndDepthShader });
	lights->addProgram({ particleSpritesDepthShader, shadedDepthShader, fluidThicknessShader });
	camera->setUniformsForAllPrograms();
	lights->setUniformsForAllPrograms();

	addParamLine(ParamLine({ &particleColor, &smoothingSize }));
	addParamLine(ParamLine({ &bilateralFilterEnabled }));
	addParamLine(ParamLine({ &blurScale, &blurDepthFalloff }, &bilateralFilterEnabled));
	addParamLine(ParamLine({ &sprayEnabled }));
	addParamLine(ParamLine({ &sprayDensityThreashold }, &sprayEnabled));
	addParamLine(ParamLine({ &fluidTransparencyEnabled }));
	addParamLine(ParamLine({ &fluidTransparencyBlurSize, &fluidTransparency }, &fluidTransparencyEnabled));
	addParamLine(ParamLine({ &fluidSurfaceNoiseEnabled }));
	addParamLine(ParamLine({ &fluidSurfaceNoiseScale, &fluidSurfaceNoiseStrength }, &fluidSurfaceNoiseEnabled));
	addParamLine(ParamLine({ &fluidSurfaceNoiseSpeed }, &fluidSurfaceNoiseEnabled));
}

void FluidSurfaceGfx::render()
{
	glm::ivec2 viewportSize = renderTargetFramebuffer->getSize();
	if (viewportSize != depthFramebuffer->getSize())
	{
		depthFramebuffer->setSize(viewportSize);
		depthBlurTmpFramebuffer->setSize(viewportSize);
		fluidThicknessBlurTmpFramebuffer->setSize(viewportSize);
		fluidThicknessFramebuffer->setSize(viewportSize);
		normalAndDepthFramebuffer->setSize(viewportSize);
	}

	updateParticleData();
	(*particleSpritesDepthShader)["particleRadius"] = simulationManager->getConfig().particleRadius;

	depthFramebuffer->bind();
	engine->setViewport(0, 0, viewportSize.x, viewportSize.y);
	engine->enableDepthTest(true);
	engine->clearViewport(1.0f);

	surfaceSquareArrayObject->draw();

	depthBlurTmpFramebuffer->bind();
	engine->clearViewport(1.0f);
	if (bilateralFilterEnabled.value)
	{
		bilateralFilterShader->activate();
		(*bilateralFilterShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
		(*bilateralFilterShader)["smoothingKernelSize"] = smoothingSize.value;
		(*bilateralFilterShader)["blurScale"] = blurScale.value;
		(*bilateralFilterShader)["blurDepthFalloff"] = blurDepthFalloff.value;
		(*bilateralFilterShader)["axis"] = 0;
		square->draw();

		depthFramebuffer->bind();
		engine->clearViewport(1.0f);
		(*bilateralFilterShader)["depthTexture"] = *depthBlurTmpFramebuffer->getDepthAttachment();
		(*bilateralFilterShader)["axis"] = 1;
		square->draw();
	}
	else
	{
		gaussianBlurShader->activate();
		(*gaussianBlurShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
		(*gaussianBlurShader)["axis"] = 0;
		(*gaussianBlurShader)["smoothingKernelSize"] = smoothingSize.value;
		square->draw();

		depthFramebuffer->bind();
		engine->clearViewport(1.0f);
		(*gaussianBlurShader)["depthTexture"] = *depthBlurTmpFramebuffer->getDepthAttachment();
		(*gaussianBlurShader)["axis"] = 1;
		square->draw();
	}

	if (sprayEnabled.value)
	{
		depthFramebuffer->bind();
		spraySquareArrayObject->draw();
	}

	if (fluidTransparencyEnabled.value)
	{
		fluidThicknessFramebuffer->bind();
		fluidThicknessShader->activate();
		(*fluidThicknessShader)["particleRadius"] = simulationManager->getConfig().particleRadius;
		engine->clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_BLEND);
		surfaceSquareArrayObject->drawable->draw();
		if (sprayEnabled.value)
			spraySquareArrayObject->drawable->draw();
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);

		fluidThicknessBlurShader->activate();
		fluidThicknessBlurTmpFramebuffer->bind();
		engine->enableDepthTest(false);
		engine->clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		(*fluidThicknessBlurShader)["thicknessTexture"] = *fluidThicknessFramebuffer->getColorAttachments()[0];
		(*fluidThicknessBlurShader)["smoothingKernelSize"] = fluidTransparencyBlurSize.value;
		(*fluidThicknessBlurShader)["axis"] = 0;
		(*fluidThicknessBlurShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
		square->draw();

		fluidThicknessFramebuffer->bind();
		engine->clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		(*fluidThicknessBlurShader)["thicknessTexture"] = *fluidThicknessBlurTmpFramebuffer->getColorAttachments()[0];
		(*fluidThicknessBlurShader)["axis"] = 1;
		square->draw();

		engine->enableDepthTest(true);
	}

	normalAndDepthFramebuffer->bind();
	engine->clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	normalAndDepthShader->activate();
	engine->enableDepthTest(false);
	square->draw();

	engine->enableDepthTest(true);
	renderTargetFramebuffer->bind();
	if (fluidTransparencyEnabled.value)
	{
		glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
		glEnable(GL_BLEND);
	}
	(*shadedDepthShader)["transparency"] = fluidTransparency.value;
	(*shadedDepthShader)["transparencyEnabled"] = fluidTransparencyEnabled.value;
	(*shadedDepthShader)["noiseEnabled"] = fluidSurfaceNoiseEnabled.value;
	if (fluidSurfaceNoiseEnabled.value)
	{
		(*shadedDepthShader)["noiseScale"] = fluidSurfaceNoiseScale.value;
		(*shadedDepthShader)["noiseStrength"] = fluidSurfaceNoiseStrength.value;
		(*shadedDepthShader)["noiseOffset"] = noiseOffset;
		noiseOffset += fluidSurfaceNoiseSpeed.value * 10.0 * engine->getLastFrameTime();
	}
	shadedSquareObject->diffuseColor = glm::vec4(particleColor.value, 1.0f);
	shadedSquareObject->draw();
	glDisable(GL_BLEND);

	engine->bindDefaultFramebuffer();
}

void FluidSurfaceGfx::setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager)
{
	this->simulationManager = simulationManager;
}

void FluidSurfaceGfx::updateParticleData()
{
	auto particles = simulationManager->getParticleGfxData();
	const int particleNum = particles.size();
	auto surfaceSquareArray = surfaceSquareArrayObject->drawable;
	auto spraySquareArray = spraySquareArrayObject->drawable;

	if (sprayEnabled.value)
	{
		int surfaceParticleCount = 0;
		int sprayParticleCount = 0;
		for (int p = 0; p < particleNum; p++)
		{

			if (particles[p].density < sprayDensityThreashold.value)
			{
				spraySquareArray->setOffset(sprayParticleCount, glm::vec4(particles[p].pos, 1.0f));
				/*if (fluidSurfaceNoiseEnabled.value)
				{
					spraySquareArray->setSpeed(sprayParticleCount, particles[p].v);
					spraySquareArray->setId(sprayParticleCount, p);
				}*/
				sprayParticleCount++;
			}
			else
			{
				surfaceSquareArray->setOffset(surfaceParticleCount, glm::vec4(particles[p].pos, 1.0f));
				/*if (fluidSurfaceNoiseEnabled.value)
				{
					surfaceSquareArray->setSpeed(surfaceParticleCount, particles[p].v);
					surfaceSquareArray->setId(surfaceParticleCount, p);
				}*/
				surfaceParticleCount++;
			}
		}
		spraySquareArray->setActiveInstanceNum(sprayParticleCount);
		surfaceSquareArray->setActiveInstanceNum(surfaceParticleCount);
		spraySquareArray->updateActiveInstanceParams();
		surfaceSquareArray->updateActiveInstanceParams();
	}
	else
	{
#pragma omp parallel for
		for (int p = 0; p < particleNum; p++)
		{
			surfaceSquareArray->setOffset(p, glm::vec4(particles[p].pos, 1.0f));
			/*if (fluidSurfaceNoiseEnabled.value)
			{
				surfaceSquareArray->setSpeed(p, particles[p].v);
				surfaceSquareArray->setId(p, p);
			}*/
		}
		surfaceSquareArray->setActiveInstanceNum(particleNum);
		surfaceSquareArray->updateActiveInstanceParams();
	}
}


