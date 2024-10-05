#include "headers/simulationGfx3DRenderer.h"

using namespace visual;

SimulationGfx3DRenderer::SimulationGfx3DRenderer(std::shared_ptr<renderer::RenderEngine> engine,
	std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights, 
	const std::vector<std::unique_ptr<renderer::Object3D<renderer::Geometry>>>& obstacleGfxArray, 
	unsigned int maxParticleNum, ConfigData3D configData)
	: engine(engine), camera(camera), lights(lights), obstacleGfxArray(obstacleGfxArray), configData(configData), 
	maxParticleNum(maxParticleNum)
{
	shaderProgramTextured = std::make_shared<renderer::ShaderProgram>("shaders/3D/3D_object.vert", "shaders/3D/3D_object_textured.frag");
	shaderProgramNotTextured = std::make_shared<renderer::ShaderProgram>("shaders/3D/3D_object.vert", "shaders/3D/3D_object_not_textured.frag");
	particleProgram = std::make_shared<renderer::ShaderProgram>("shaders/3D/particleArray.vert", "shaders/3D/particleArray.frag");
	particleProgram_id = std::make_shared<renderer::ShaderProgram>("shaders/3D/particleArray.vert", "shaders/3D/particleArray_id.frag");

	particlesGfx = std::make_unique<renderer::Object3D<renderer::InstancedGeometry>>
		(std::make_shared<renderer::InstancedGeometry>(std::make_shared<renderer::Sphere>(8)), particleProgram);
	particlesGfx->shininess = 80;
	particlesGfx->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);

	paramTmpTexture = std::make_shared<renderer::RenderTargetTexture>
		(1000, 1000, GL_NEAREST, GL_NEAREST, GL_R32I, GL_RED_INTEGER, GL_INT);

	paramCopyProgram = std::make_shared<renderer::ComputeProgram>("shaders/3D/util/copyParams.comp");
	(*paramCopyProgram)["paramTexture"] = *paramTmpTexture;

	auto floorTexture = std::make_shared<renderer::ColorTexture>("shaders/3D/tiles.jpg", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
	auto floor = std::make_shared<renderer::Square>();
	planeGfx = std::make_unique<renderer::Object3D<renderer::Geometry>>(floor, shaderProgramTextured);
	planeGfx->colorTextureScale = 5.0;
	planeGfx->colorTexture = floorTexture;
	planeGfx->shininess = 5.8;
	planeGfx->specularColor = glm::vec4(0.7, 0.7, 0.7, 1);
	planeGfx->diffuseColor = glm::vec4(1, 1, 1, 1);
	planeGfx->setScale(glm::vec3(200, 200, 1));
	planeGfx->setPitch(PI * 0.5f);

	transparentBox = std::make_unique<TransparentBox>(camera, glm::vec4(0.5, 0.5, 0.65, 0.4), shaderProgramNotTextured);

	camera->addProgram({ shaderProgramTextured, shaderProgramNotTextured, particleProgram, particleProgram_id });
	lights->addProgram({ shaderProgramTextured, shaderProgramNotTextured, particleProgram, particleProgram_id });
	camera->setUniformsForAllPrograms();
	lights->setUniformsForAllPrograms();

	addParamLine({ &showFloor, &showBox, &showObstacles });
	addParamLine({ &fluidRenderMode });
}

void SimulationGfx3DRenderer::setConfigData(const ConfigData3D& configData)
{
	this->configData = configData;
}

void SimulationGfx3DRenderer::show(int screenWidth)
{
	ImGui::SeparatorText("Renderer 3D");
	ParamLineCollection::show(screenWidth);
	if(fluidRenderMode.value == SURFACE && fluidSurfaceGfx)
		fluidSurfaceGfx->show(screenWidth);
}

void visual::SimulationGfx3DRenderer::invalidateParamBuffer()
{
	if (fluidSurfaceGfx)
		fluidSurfaceGfx->invalidateParamBuffer();
	else
		ParamInterface::invalidateParamBuffer();
}

void SimulationGfx3DRenderer::render(std::shared_ptr<renderer::Framebuffer> framebuffer, renderer::ssbo_ptr<ParticleShaderData> data)
{
	handleFluidRenderModeChange();

	framebuffer->bind();

	engine->enableDepthTest(true);
	engine->setViewport(0, 0, configData.screenSize.x, configData.screenSize.y);
	engine->clearViewport(glm::vec4(0, 0, 0, 0), 1.0f);

	if (showFloor.value)
	{
		planeGfx->setPosition(glm::vec4(configData.sceneCenter.x, -0.05f, configData.sceneCenter.z, 1));
		planeGfx->draw();
	}

	if (showObstacles.value)
	{
		for (auto& obstacle : obstacleGfxArray)
		{
			obstacle->draw();
		}
	}

	if (showBox.value)
	{
		transparentBox->draw(configData.sceneCenter, configData.boxSize, true, false);
	}

	if(fluidRenderMode.value == PARTICLES)
	{
		renderParticles(framebuffer, data);
	}
	else if (fluidRenderMode.value == SURFACE)
	{
		fluidSurfaceGfx->render(framebuffer, data);
	}

	framebuffer->bind();
	if (showBox.value)
	{
		transparentBox->draw(configData.sceneCenter, configData.boxSize, false, true);
	}
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void visual::SimulationGfx3DRenderer::renderParticles(std::shared_ptr<renderer::Framebuffer> framebuffer,
	renderer::ssbo_ptr<ParticleShaderData> data)
{
	const float maxSpeedInv = 1.0f / configData.maxParticleSpeed;
	const unsigned int numParticles = data->getSize();
	particlesGfx->drawable->setInstanceNum(numParticles);
	particlesGfx->setScale(glm::vec3(configData.particleRadius, configData.particleRadius, configData.particleRadius));
	/*if (configData.speedColorEnabled || configData.color != glm::vec3(ballGeometry->getColor(0)))
	{
		for (unsigned int i = 0; i < numParticles; i++)
		{
			float s = std::clamp(data[i].v * maxSpeedInv, 0.0f, 1.0f);
			s = std::pow(s, 0.3f);
			ballGeometry->setColor(i, glm::vec4((configData.color * (1.0f - s)) + (configData.speedColor * s), 1));
		}
	}*/

	if (paramBufferValid)
	{
		particlesGfx->shaderProgram = particleProgram;
	}
	else
	{
		particlesGfx->shaderProgram = particleProgram_id;
	}

	data->bindBuffer(60);

	(*particlesGfx->shaderProgram)["speedColorEnabled"] = configData.speedColorEnabled;
	(*particlesGfx->shaderProgram)["maxSpeedInv"] = maxSpeedInv;
	(*particlesGfx->shaderProgram)["color"] = configData.color;
	(*particlesGfx->shaderProgram)["speedColor"] = configData.speedColor;

	if (paramBufferValid)
	{
		framebuffer->bind();
		particlesGfx->draw();
	}
	else
	{
		paramBufferValid = true;
		auto colorAttachment = framebuffer->getColorAttachments()[0];
		framebuffer->setColorAttachments(renderer::Framebuffer::toArray({ colorAttachment, paramTmpTexture }));
		framebuffer->clearColorAttachment(1, glm::ivec4(-1));
		framebuffer->bind();
		particlesGfx->draw();
		framebuffer->setColorAttachments(renderer::Framebuffer::toArray({ colorAttachment }));
		framebuffer->bind();

		(*paramCopyProgram)["screenSize"] = configData.screenSize;
		ParamInterface::getParamBufferOut()->bindBuffer(30);
		paramCopyProgram->dispatchCompute(configData.screenSize.x, configData.screenSize.y, 1);
	}
}

std::shared_ptr<renderer::StorageBuffer<ParamInterface::PixelParamData>> visual::SimulationGfx3DRenderer::getParamBufferOut() const
{
	if(fluidSurfaceGfx)
		return fluidSurfaceGfx->getParamBufferOut();
	return ParamInterface::getParamBufferOut();
}

void visual::SimulationGfx3DRenderer::handleFluidRenderModeChange()
{
	if (fluidRenderMode.value == SURFACE)
	{
		deleteParamBuffer();
		if (!fluidSurfaceGfx)
		{
			fluidSurfaceGfx = std::make_unique<FluidSurfaceGfx>(engine, camera, lights, maxParticleNum);
			fluidSurfaceGfx->setConfigData(configData);
		}
	}
	else if(fluidRenderMode.value == PARTICLES)
	{
		fluidSurfaceGfx.reset();
		setBufferSize(configData.screenSize);
		paramTmpTexture->resizeTexture(configData.screenSize.x, configData.screenSize.y);
	}
	else
	{
		deleteParamBuffer();
		fluidSurfaceGfx.reset();
	}
}




