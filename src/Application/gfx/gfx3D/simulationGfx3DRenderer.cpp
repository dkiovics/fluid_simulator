#include "simulationGfx3DRenderer.h"

using namespace gfx3D;

SimulationGfx3DRenderer::SimulationGfx3DRenderer(std::shared_ptr<renderer::RenderEngine> engine,
	std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights, 
	const std::vector<std::unique_ptr<renderer::Object3D<renderer::Geometry>>>& obstacleGfxArray, 
	unsigned int maxParticleNum, ConfigData3D configData)
	: engine(engine), camera(camera), lights(lights), obstacleGfxArray(obstacleGfxArray), configData(configData), 
	maxParticleNum(maxParticleNum)
{
	shaderProgramTextured = std::make_shared<renderer::ShaderProgram>("shaders/3D_object.vert", "shaders/3D_object_textured.frag");
	shaderProgramNotTextured = std::make_shared<renderer::ShaderProgram>("shaders/3D_object.vert", "shaders/3D_object_not_textured.frag");
	shaderProgramNotTexturedArray = std::make_shared<renderer::ShaderProgram>("shaders/3D_objectArray.vert", "shaders/3D_objectArray.frag");
	shaderProgramNotTexturedArrayWithId = std::make_shared<renderer::ShaderProgram>("shaders/3D_objectArray.vert", "shaders/3D_objectArray_id.frag");

	auto floorTexture = std::make_shared<renderer::ColorTexture>("shaders/tiles.jpg", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
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

	camera->addProgram({ shaderProgramTextured, shaderProgramNotTextured, shaderProgramNotTexturedArray, shaderProgramNotTexturedArrayWithId });
	lights->addProgram({ shaderProgramTextured, shaderProgramNotTextured, shaderProgramNotTexturedArray, shaderProgramNotTexturedArrayWithId });
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

void SimulationGfx3DRenderer::render(std::shared_ptr<renderer::Framebuffer> framebuffer, 
	std::shared_ptr<renderer::RenderTargetTexture> paramTexture, const Gfx3DRenderData& data)
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
		renderParticles(framebuffer, paramTexture, data.particleData);
	}
	else if (fluidRenderMode.value == SURFACE)
	{
		fluidSurfaceGfx->render(framebuffer, paramTexture, data);
	}

	if (showBox.value)
	{
		transparentBox->draw(configData.sceneCenter, configData.boxSize, false, true);
	}
}

void gfx3D::SimulationGfx3DRenderer::renderParticles(std::shared_ptr<renderer::Framebuffer> framebuffer,
	std::shared_ptr<renderer::RenderTargetTexture> paramTexture, 
	const std::vector<genericfsim::manager::SimulationManager::ParticleGfxData>& data)
{
	auto ballGeometry = particlesGfx->drawable;
	const float maxSpeedInv = 1.0f / configData.maxParticleSpeed;
	const unsigned int numParticles = data.size();
	ballGeometry->setActiveInstanceNum(data.size());
	for (unsigned int i = 0; i < numParticles; i++)
	{
		ballGeometry->setOffset(i, glm::vec4(data[i].pos, 0));
	}
	if (configData.speedColorEnabled || configData.color != glm::vec3(ballGeometry->getColor(0)))
	{
		for (unsigned int i = 0; i < numParticles; i++)
		{
			float s = std::clamp(data[i].v * maxSpeedInv, 0.0f, 1.0f);
			s = std::pow(s, 0.3f);
			ballGeometry->setColor(i, glm::vec4((configData.color * (1.0f - s)) + (configData.speedColor * s), 1));
		}
	}

	ballGeometry->updateActiveInstanceParams();
	particlesGfx->setScale(glm::vec3(configData.particleRadius, configData.particleRadius, configData.particleRadius));
	if (paramTexture)
	{
		particlesGfx->shaderProgram = shaderProgramNotTexturedArrayWithId;
		auto colorAttachment = framebuffer->getColorAttachments()[0];
		framebuffer->setColorAttachments(renderer::Framebuffer::toArray({ colorAttachment, paramTexture }));
		framebuffer->bind();
		particlesGfx->draw();
		framebuffer->setColorAttachments(renderer::Framebuffer::toArray({ colorAttachment }));
		framebuffer->bind();
	}
	else
	{
		particlesGfx->shaderProgram = shaderProgramNotTexturedArray;
		particlesGfx->draw();
	}
}

void gfx3D::SimulationGfx3DRenderer::handleFluidRenderModeChange()
{
	if (fluidRenderMode.value == SURFACE)
	{
		particlesGfx.reset();
		if (!fluidSurfaceGfx)
		{
			fluidSurfaceGfx = std::make_unique<FluidSurfaceGfx>(engine, camera, lights, maxParticleNum);
			fluidSurfaceGfx->setConfigData(configData);
		}
	}
	else if(fluidRenderMode.value == PARTICLES)
	{
		fluidSurfaceGfx.reset();
		if (!particlesGfx)
		{
			fluidSurfaceGfx.reset();
			auto spheres = std::make_shared<renderer::BasicGeometryArray>(std::make_shared<renderer::Sphere>(8));
			spheres->setMaxInstanceNum(maxParticleNum);
			particlesGfx = std::make_unique<renderer::Object3D<renderer::BasicGeometryArray>>(spheres, shaderProgramNotTexturedArray);
			particlesGfx->shininess = 80;
			particlesGfx->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);
		}
	}
	else
	{
		fluidSurfaceGfx.reset();
		particlesGfx.reset();
	}
}




