#include "fluidSurfaceGfx.h"

FluidSurfaceGfx::FluidSurfaceGfx(std::shared_ptr<renderer::RenderEngine> engine, 
	std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager, 
	std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights, unsigned int maxParticleNum)
	: engine(engine), simulationManager(simulationManager), camera(camera), lights(lights)
{
	pointSpritesShader = std::make_shared<renderer::ShaderProgram>("shaders/particle_sprites.vs", "shaders/particle_sprites.fs", engine);
	normalShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/normals.fs", engine);
	gaussianBlurShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/gaussian.fs", engine);
	shadedDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/shadedDepth.fs", engine);

	squareArrayObject = std::make_unique<renderer::Object3D<renderer::BasicGeometryArray>>
		(std::make_shared<renderer::BasicGeometryArray>(std::make_shared<renderer::FlipSquare>()), pointSpritesShader);
	squareArrayObject->drawable->setMaxInstanceNum(maxParticleNum);
	squareArrayObject->shininess = 80;
	squareArrayObject->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);

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

	camera->addProgram({ pointSpritesShader });
	camera->addProgram({ normalShader });
	camera->addProgram({ gaussianBlurShader });
	camera->addProgram({ shadedDepthShader });
	lights->addProgram({ pointSpritesShader });
	lights->addProgram({ shadedDepthShader });
	camera->setUniformsForAllPrograms();
	lights->setUniformsForAllPrograms();

	addParamLine(ParamLine({ &smoothingSize, &particleColor }));
	addParamLine(ParamLine({ &particleSpeedColorEnabled, &particleSpeedColor, &maxParticleSpeed }));
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
	(*pointSpritesShader)["particleRadius"] = simulationManager->getConfig().particleRadius;

	depthFramebuffer->bind();
	engine->setViewport(0, 0, viewportSize.x, viewportSize.y);
	engine->enableDepthTest(true);
	engine->clearViewport(1.0f);

	squareArrayObject->draw();

	depthBlurTmpFramebuffer->bind();
	engine->clearViewport(1.0f);
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

	renderTargetFramebuffer->bind();
	shadedSquareObject->diffuseColor = glm::vec4(particleColor.value, 1.0f);
	shadedSquareObject->draw();

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
	const float maxSpeedInv = 1.0f / maxParticleSpeed.value;
	auto squareArray = squareArrayObject->drawable;
	squareArray->setActiveInstanceNum(particleNum);

#pragma omp parallel for
	for (int p = 0; p < particleNum; p++)
	{
		squareArray->setOffset(p, glm::vec4(particles[p].pos, 0));
		if (particleSpeedColorEnabled.value)
		{
			float s = std::min(particles[p].v * maxSpeedInv, 1.0f);
			s = std::pow(s, 0.3f);
			squareArray->setColor(p, glm::vec4((particleColor.value * (1.0f - s)) + (particleSpeedColor.value * s), 1));
		}
	}
	if (particleSpeedColorEnabled.value)
	{
		particleSpeedColorWasEnabled = true;
	}
	if (!particleSpeedColorEnabled.value && 
		(particleSpeedColorWasEnabled || prevColor != particleColor.value || particleNum != prevParticleNum))
	{
		prevParticleNum = particleNum;
		particleSpeedColorWasEnabled = false;
		prevColor = particleColor.value;
		for (int p = 0; p < particleNum; p++)
		{
			squareArray->setColor(p, glm::vec4(particleColor.value, 1));
		}
	}
	squareArray->updateActiveInstanceParams();
}


