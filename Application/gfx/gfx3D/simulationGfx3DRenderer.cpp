#include "simulationGfx3DRenderer.h"

using namespace gfx3D;

SimulationGfx3DRenderer::SimulationGfx3DRenderer(std::shared_ptr<renderer::RenderEngine> engine, std::shared_ptr<renderer::Camera3D> camera, 
	std::shared_ptr<renderer::Lights> lights, const std::vector<std::unique_ptr<renderer::Object3D<renderer::Geometry>>>& obstacleGfxArray, 
	unsigned int maxParticleNum, ConfigData3D configData)
	: engine(engine), camera(camera), lights(lights), obstacleGfxArray(obstacleGfxArray), configData(configData)
{
	shaderProgramTextured = std::make_shared<renderer::ShaderProgram>("shaders/3D_object.vert", "shaders/3D_object_textured.frag");
	shaderProgramNotTextured = std::make_shared<renderer::ShaderProgram>("shaders/3D_object.vert", "shaders/3D_object_not_textured.frag");
	shaderProgramNotTexturedArray = std::make_shared<renderer::ShaderProgram>("shaders/3D_objectArray.vert", "shaders/3D_objectArray.frag");

	auto floorTexture = std::make_shared<renderer::ColorTexture>("shaders/tiles.jpg", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
	//auto floor = std::make_shared<renderer::Square>(1, std::vector<glm::vec3>{ glm::vec3(0, -0.1, 0) }, std::vector<glm::vec4>{ glm::vec4(1, 1, 1, 1) }, engine, 200, 200);
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

	auto spheres = std::make_shared<renderer::BasicGeometryArray>(std::make_shared<renderer::Sphere>(8));
	spheres->setMaxInstanceNum(maxParticleNum);
	ballsGfx = std::make_unique<renderer::Object3D<renderer::BasicGeometryArray>>(spheres, shaderProgramNotTexturedArray);
	ballsGfx->shininess = 80;
	ballsGfx->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);

	camera->addProgram({ shaderProgramTextured, shaderProgramNotTextured, shaderProgramNotTexturedArray });
	lights->addProgram({ shaderProgramTextured, shaderProgramNotTextured, shaderProgramNotTexturedArray });
	camera->setUniformsForAllPrograms();
	lights->setUniformsForAllPrograms();
}

void SimulationGfx3DRenderer::setConfigData(const ConfigData3D& configData)
{
	this->configData = configData;
}

void SimulationGfx3DRenderer::render(std::shared_ptr<renderer::Framebuffer> framebuffer, const Gfx3DRenderData& renderData) const
{
	auto ballGeometry = ballsGfx->drawable;
	const float maxSpeedInv = 1.0f / configData.maxParticleSpeed;
	const unsigned int numParticles = renderData.positions.size();
	ballGeometry->setActiveInstanceNum(renderData.positions.size());
	if (renderData.positionsChanged)
	{
		for(unsigned int i = 0; i < numParticles; i++)
		{
			ballGeometry->setOffset(i, glm::vec4(renderData.positions[i], 0));
		}
	}
	if(renderData.speedOrColorChanged)
	{
		for (unsigned int i = 0; i < numParticles; i++)
		{
			float s = std::min(renderData.speeds[i] * maxSpeedInv, 1.0f);
			s = std::pow(s, 0.3f);
			ballGeometry->setColor(i, glm::vec4((renderData.color * (1.0f - s)) + (renderData.speedColor * s), 1));
		}
	}

	framebuffer->bind();

	engine->enableDepthTest(true);
	engine->setViewport(0, 0, configData.screenSize.x, configData.screenSize.y);
	engine->clearViewport(glm::vec4(0.1, 0, 0, 0), 1.0f);

	planeGfx->setPosition(glm::vec4(configData.sceneCenter.x, -0.05f, configData.sceneCenter.z, 1));
	planeGfx->draw();

	for (auto& obstacle : obstacleGfxArray)
	{
		obstacle->draw();
	}

	transparentBox->draw(configData.sceneCenter, configData.boxSize, true, false);

	ballGeometry->updateActiveInstanceParams();
	ballsGfx->setScale(glm::vec3(configData.particleRadius, configData.particleRadius, configData.particleRadius));
	ballsGfx->draw();

	transparentBox->draw(configData.sceneCenter, configData.boxSize, false, true);
}




