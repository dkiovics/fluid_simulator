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

	surfaceSquareArrayObject = std::make_unique<renderer::Object3D<renderer::BasicPosGeometryArray>>
		(std::make_shared<renderer::BasicPosGeometryArray>(std::make_shared<renderer::FlipSquare>()), particleSpritesDepthShader);
	surfaceSquareArrayObject->drawable->setMaxInstanceNum(maxParticleNum);

	spraySquareArrayObject = std::make_unique<renderer::Object3D<renderer::BasicPosGeometryArray>>
		(std::make_shared<renderer::BasicPosGeometryArray>(std::make_shared<renderer::FlipSquare>()), particleSpritesDepthShader);
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
	}

	{
		std::shared_ptr<renderer::RenderTargetTexture> fluidThicknessTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);
		std::vector<std::shared_ptr<renderer::RenderTargetTexture>> fluidThicknessTextures = { fluidThicknessTexture };
		fluidThicknessFramebuffer = std::make_unique<renderer::Framebuffer>
			(fluidThicknessTextures, renderTargetFramebuffer->getDepthAttachment(), false);
		std::shared_ptr<renderer::RenderTargetTexture> fluidThicknessBlurTmpTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);
		std::vector<std::shared_ptr<renderer::RenderTargetTexture>> fluidThicknessBlurTmpTextures = { fluidThicknessBlurTmpTexture };
		fluidThicknessBlurTmpFramebuffer = std::make_unique<renderer::Framebuffer>
			(fluidThicknessBlurTmpTextures, nullptr, false);
	}

	(*shadedDepthShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();

	camera->addProgram({ particleSpritesDepthShader, gaussianBlurShader, 
		bilateralFilterShader, shadedDepthShader, fluidThicknessShader, fluidThicknessBlurShader });
	lights->addProgram({ particleSpritesDepthShader, shadedDepthShader, fluidThicknessShader });
	camera->setUniformsForAllPrograms();
	lights->setUniformsForAllPrograms();

	addParamLine(ParamLine({ &particleColor }));
	addParamLine(ParamLine({ &bilateralFilterEnabled, &smoothingSize }));
	addParamLine(ParamLine({ &blurScale, &blurDepthFalloff }));
	addParamLine(ParamLine({ &sprayEnabled, &sprayDensityThreashold }));
	addParamLine(ParamLine({ &fluidTransparencyEnabled }));
	addParamLine(ParamLine({ &fluidTransparencyBlurSize, &fluidTransparency }));
}

void FluidSurfaceGfx::render()
{
	glm::ivec2 viewportSize = renderTargetFramebuffer->getSize();
	if (viewportSize != depthFramebuffer->getSize())
	{
		depthFramebuffer->setSize(viewportSize);
		depthBlurTmpFramebuffer->setSize(viewportSize);
		fluidThicknessFramebuffer->setSize(viewportSize);
		fluidThicknessBlurTmpFramebuffer->setSize(viewportSize);
	}

	updateParticleColorsAndPositions();
	(*particleSpritesDepthShader)["particleRadius"] = simulationManager->getConfig().particleRadius;
	(*fluidThicknessShader)["particleRadius"] = simulationManager->getConfig().particleRadius;

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
		engine->clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_BLEND);
		surfaceSquareArrayObject->drawable->draw();
		if(sprayEnabled.value)
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

	renderTargetFramebuffer->bind();
	if (fluidTransparencyEnabled.value)
	{
		glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
		glEnable(GL_BLEND);
	}
	(*shadedDepthShader)["thicknessTexture"] = *fluidThicknessFramebuffer->getColorAttachments()[0];
	(*shadedDepthShader)["transparency"] = fluidTransparency.value;
	(*shadedDepthShader)["transparencyEnabled"] = fluidTransparencyEnabled.value ? 1 : 0;
	shadedSquareObject->diffuseColor = glm::vec4(particleColor.value, 1.0f);
	shadedSquareObject->draw();
	glDisable(GL_BLEND);

	engine->bindDefaultFramebuffer();
}

void FluidSurfaceGfx::setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager)
{
	this->simulationManager = simulationManager;
}

void FluidSurfaceGfx::updateParticleColorsAndPositions()
{
	auto particles = simulationManager->getParticleGfxData();
	const int particleNum = particles.size();
	auto surfaceSquareArray = surfaceSquareArrayObject->drawable;
	auto spraySquareArray = spraySquareArrayObject->drawable;
	simulationManager->setCalculateParticleDensities(sprayEnabled.value);
	simulationManager->setCalculateParticleSpeeds(false);

	if (sprayEnabled.value)
	{
		int surfaceParticleCount = 0;
		int sprayParticleCount = 0;
		for (int p = 0; p < particleNum; p++)
		{

			if (particles[p].density < sprayDensityThreashold.value)
			{
				spraySquareArray->setOffset(sprayParticleCount, glm::vec4(particles[p].pos, 1.0f));
				sprayParticleCount++;
			}
			else
			{
				surfaceSquareArray->setOffset(surfaceParticleCount, glm::vec4(particles[p].pos, 1.0f));
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
		}
		surfaceSquareArray->setActiveInstanceNum(particleNum);
		surfaceSquareArray->updateActiveInstanceParams();
	}
}


