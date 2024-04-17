#include "fluidSurfaceGfx.h"

FluidSurfaceGfx::FluidSurfaceGfx(std::shared_ptr<renderer::RenderEngine> engine, 
	std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager, 
	std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights, unsigned int maxParticleNum)
	: engine(engine), simulationManager(simulationManager), camera(camera), lights(lights)
{
	particleSpritesDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/particle_sprites.vs", "shaders/particle_sprites_depth.fs", engine);
	normalShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/normals.fs", engine);
	gaussianBlurShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/gaussian.fs", engine);
	shadedDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/shadedDepth.fs", engine);
	bilateralFilterShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/bilateral.fs", engine);
	particleSpritesShadedShader = std::make_shared<renderer::ShaderProgram>("shaders/particle_sprites.vs", "shaders/particle_sprites_shaded.fs", engine);

	surfaceSquareArrayObject = std::make_unique<renderer::Object3D<renderer::BasicPosGeometryArray>>
		(std::make_shared<renderer::BasicPosGeometryArray>(std::make_shared<renderer::FlipSquare>()), particleSpritesDepthShader);
	surfaceSquareArrayObject->drawable->setMaxInstanceNum(maxParticleNum);
	surfaceSquareArrayObject->shininess = 80;
	surfaceSquareArrayObject->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);

	spraySquareArrayObject = std::make_unique<renderer::Object3D<renderer::BasicPosGeometryArray>>
		(std::make_shared<renderer::BasicPosGeometryArray>(std::make_shared<renderer::FlipSquare>()), particleSpritesShadedShader);
	spraySquareArrayObject->drawable->setMaxInstanceNum(maxParticleNum);
	spraySquareArrayObject->shininess = 80;
	spraySquareArrayObject->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);

	shadedSquareObject = std::make_unique<renderer::Object3D<renderer::Square>>(std::make_shared<renderer::Square>(), shadedDepthShader);
	shadedSquareObject->shininess = 80;
	shadedSquareObject->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);

	square = std::make_unique<renderer::Square>();

	depthTexture = std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST,
		GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
	depthFramebuffer = std::make_unique<renderer::Framebuffer>
		(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>(), depthTexture);
	depthBlurTmpTexture = std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST,
		GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
	depthBlurTmpFramebuffer = std::make_unique<renderer::Framebuffer>
		(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>(), depthBlurTmpTexture);

	(*normalShader)["depthTexture"] = *depthTexture;
	(*shadedDepthShader)["depthTexture"] = *depthTexture;

	camera->addProgram({ particleSpritesDepthShader, particleSpritesShadedShader, normalShader, 
		gaussianBlurShader, bilateralFilterShader, shadedDepthShader });
	lights->addProgram({ particleSpritesDepthShader, particleSpritesShadedShader, shadedDepthShader });
	camera->setUniformsForAllPrograms();
	lights->setUniformsForAllPrograms();

	addParamLine(ParamLine({ &particleColor }));
	addParamLine(ParamLine({ &bilateralFilterEnabled, &smoothingSize }));
	addParamLine(ParamLine({ &blurScale, &blurDepthFalloff }));
	addParamLine(ParamLine({ &sprayEnabled, &sprayDensityThreashold }));
}

void FluidSurfaceGfx::render(std::shared_ptr<renderer::Framebuffer> renderTargetFramebuffer)
{
	glm::ivec2 viewportSize = renderTargetFramebuffer->getSize();
	if (viewportSize != depthFramebuffer->getSize())
	{
		depthFramebuffer->setSize(viewportSize);
		depthBlurTmpFramebuffer->setSize(viewportSize);
	}

	updateParticleColorsAndPositions();
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
		(*bilateralFilterShader)["depthTexture"] = *depthTexture;
		(*bilateralFilterShader)["smoothingKernelSize"] = smoothingSize.value;
		(*bilateralFilterShader)["blurScale"] = blurScale.value;
		(*bilateralFilterShader)["blurDepthFalloff"] = blurDepthFalloff.value;
		(*bilateralFilterShader)["axis"] = 0;
		square->draw();

		depthFramebuffer->bind();
		engine->clearViewport(1.0f);
		(*bilateralFilterShader)["depthTexture"] = *depthBlurTmpTexture;
		(*bilateralFilterShader)["axis"] = 1;
		square->draw();
	}
	else
	{
		gaussianBlurShader->activate();
		(*gaussianBlurShader)["depthTexture"] = *depthTexture;
		(*gaussianBlurShader)["axis"] = 0;
		(*gaussianBlurShader)["smoothingKernelSize"] = smoothingSize.value;
		square->draw();

		depthFramebuffer->bind();
		engine->clearViewport(1.0f);
		(*gaussianBlurShader)["depthTexture"] = *depthBlurTmpTexture;
		(*gaussianBlurShader)["axis"] = 1;
		square->draw();
	}

	renderTargetFramebuffer->bind();
	shadedSquareObject->diffuseColor = glm::vec4(particleColor.value, 1.0f);
	shadedSquareObject->draw();

	if (sprayEnabled.value)
	{
		(*particleSpritesShadedShader)["particleRadius"] = simulationManager->getConfig().particleRadius;
		spraySquareArrayObject->diffuseColor = glm::vec4(particleColor.value, 1.0f);
		spraySquareArrayObject->draw();
	}

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


