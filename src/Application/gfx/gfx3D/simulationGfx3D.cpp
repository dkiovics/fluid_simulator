#include "simulationGfx3D.h"
#include <algorithm>
#include <geometries/basicGeometries.h>
#include <map>

void SimulationGfx3D::mouseCallback(double x, double y)
{
	mousePos = glm::vec2((x - screenStart.x) / screenSize.x, (screenStart.y - y) / screenSize.y + 1.0f);
	if (mousePos.x < 0 || mousePos.x > 1 || mousePos.y < 0 || mousePos.y > 1)
	{
		mouseValid = false;
		isLeftButtonDown = false;
		isRightButtonDown = false;
		isMiddleButtonDown = false;
		selectedObstacle = -1;
		return;
	}
	mouseValid = true;
	glm::vec2 d = mousePos - prevMousePos;
	d = d * 50.0f;
	if (isLeftButtonDown)
	{
		camera->incrementPitchAndYaw(d.y, d.x);
	}
	else if (isMiddleButtonDown)
	{
		camera->rotateAroundPoint(modelRotationPoint, cameraDistance, d.y, d.x);
	}
	else if (isRightButtonDown)
	{
		handleObstacleMovement();
	}
	prevMousePos = mousePos;
}

void SimulationGfx3D::mouseButtonCallback(int button, int action, int mods)
{
	if (!mouseValid)
		return;
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			isLeftButtonDown = true;
			inModelRotationMode = false;
		}
		else if (action == GLFW_RELEASE)
			isLeftButtonDown = false;
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		if (action == GLFW_PRESS)
		{
			isMiddleButtonDown = true;
			inModelRotationMode = true;
			camera->rotateAroundPoint(modelRotationPoint, cameraDistance, 0, 0);
		}
		else if (action == GLFW_RELEASE)
			isMiddleButtonDown = false;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			isRightButtonDown = true;
			selectObstacle();
		}
		else if (action == GLFW_RELEASE)
		{
			isRightButtonDown = false;
			selectedObstacle = -1;
		}
	}
}

void SimulationGfx3D::scrollCallback(double xoffset, double yoffset)
{
	if (!mouseValid)
		return;
	if (inModelRotationMode)
	{
		if (yoffset < 0)
		{
			cameraDistance = std::clamp(cameraDistance + 5.0f, minCameraDistance, maxCameraDIstance);
		}
		else if (yoffset > 0)
		{
			cameraDistance = std::clamp(cameraDistance - 5.0f, minCameraDistance, maxCameraDIstance);
		}
		camera->rotateAroundPoint(modelRotationPoint, cameraDistance, 0, 0);
	}
}

void SimulationGfx3D::keyCallback(int key, int scancode, int action, int mode)
{
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			keys[key] = true;
			inModelRotationMode = false;
		}
		else if (action == GLFW_RELEASE)
		{
			keys[key] = false;
		}
	}
}

void SimulationGfx3D::handleScreenChanged()
{
	renderTargetFramebuffer->setSize(glm::ivec2(screenSize.x, screenSize.y));
	camera->setAspectRatio(float(screenSize.x) / screenSize.y);
}

void SimulationGfx3D::drawParticles()
{
	auto particles = simulationManager->getParticleGfxData();
	auto& geometry = *(ballsGfx->drawable);
	const int particleNum = particles.size();
	const float maxSpeedInv = 1.0f / maxParticleSpeed.value;
	geometry.setActiveInstanceNum(particleNum);

	for (int p = 0; p < particleNum; p++)
	{
		geometry.setOffset(p, glm::vec4(particles[p].pos, 0));
		if (particleSpeedColorEnabled.value)
		{
			float s = std::min(particles[p].v * maxSpeedInv, 1.0f);
			s = std::pow(s, 0.3f);
			geometry.setColor(p, glm::vec4((particleColor.value * (1.0f - s)) + (particleSpeedColor.value * s), 1));
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
			geometry.setColor(p, glm::vec4(particleColor.value, 1));
		}
	}

	geometry.updateActiveInstanceParams();

	float r = simulationManager->getConfig().particleRadius;
	ballsGfx->setScale(glm::vec3(r, r, r));
	ballsGfx->draw();
}

void SimulationGfx3D::initGridLines()
{
	constexpr double gridlineWidthCoef = 0.0004;
	const glm::ivec3 gridSize = simulationManager->getGridSize();
	const glm::dvec3 cellD = simulationManager->getCellD();
	const glm::dvec3 dimensions = simulationManager->getDimensions();

	{
		std::vector<glm::vec4> linesXPos;
		std::vector<glm::vec4> color;
		for (int y = 1; y < gridSize.y; y++)
		{
			for (int z = 1; z < gridSize.z; z++)
			{
				linesXPos.push_back(glm::vec4(dimensions.x * 0.5, y * cellD.y, z * cellD.z, 0));
				color.push_back(glm::vec4(gridLineColor.value, 1));
			}
		}
		std::shared_ptr<renderer::Cube> linesGeometry = std::make_shared<renderer::Cube>();
		gridLinesXGfx = std::make_unique<renderer::Object3D<renderer::BasicGeometryArray>>(std::make_shared<renderer::BasicGeometryArray>(linesGeometry), shaderProgramNotTexturedArray);
		int num = linesXPos.size();
		gridLinesXGfx->drawable->setMaxInstanceNum(linesXPos.size(), std::move(linesXPos), std::move(color));
		gridLinesXGfx->drawable->setActiveInstanceNum(num);
		gridLinesXGfx->setScale(glm::vec3(dimensions.x, dimensions.y * gridlineWidthCoef, dimensions.z * gridlineWidthCoef));
		gridLinesXGfx->shininess = 6.0;
		gridLinesXGfx->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);
		gridLinesXGfx->drawable->updateActiveInstanceParams();
	}
	{
		std::vector<glm::vec4> linesYPos;
		std::vector<glm::vec4> color;
		for (int x = 1; x < gridSize.x; x++)
		{
			for (int z = 1; z < gridSize.z; z++)
			{
				linesYPos.push_back(glm::vec4(x * cellD.x, dimensions.y * 0.5, z * cellD.z, 0));
				color.push_back(glm::vec4(gridLineColor.value, 1));
			}
		}
		std::shared_ptr<renderer::Cube> linesGeometry = std::make_shared<renderer::Cube>();
		gridLinesYGfx = std::make_unique<renderer::Object3D<renderer::BasicGeometryArray>>(std::make_shared<renderer::BasicGeometryArray>(linesGeometry), shaderProgramNotTexturedArray);
		int num = linesYPos.size();
		gridLinesYGfx->drawable->setMaxInstanceNum(linesYPos.size(), std::move(linesYPos), std::move(color));
		gridLinesYGfx->drawable->setActiveInstanceNum(num);
		gridLinesYGfx->setScale(glm::vec3(dimensions.x * gridlineWidthCoef, dimensions.y, dimensions.z * gridlineWidthCoef));
		gridLinesYGfx->shininess = 6.0;
		gridLinesYGfx->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);
		gridLinesYGfx->drawable->updateActiveInstanceParams();
	}
	{
		std::vector<glm::vec4> linesZPos;
		std::vector<glm::vec4> color;
		for (int x = 1; x < gridSize.x; x++)
		{
			for (int y = 1; y < gridSize.y; y++)
			{
				linesZPos.push_back(glm::vec4(x * cellD.x, y * cellD.y, dimensions.z * 0.5, 0));
				color.push_back(glm::vec4(gridLineColor.value, 1));
			}
		}
		std::shared_ptr<renderer::Cube> linesGeometry = std::make_shared<renderer::Cube>();
		gridLinesZGfx = std::make_unique<renderer::Object3D<renderer::BasicGeometryArray>>(std::make_shared<renderer::BasicGeometryArray>(linesGeometry), shaderProgramNotTexturedArray);
		int num = linesZPos.size();
		gridLinesZGfx->drawable->setMaxInstanceNum(linesZPos.size(), std::move(linesZPos), std::move(color));
		gridLinesZGfx->drawable->setActiveInstanceNum(num);
		gridLinesZGfx->setScale(glm::vec3(dimensions.x * gridlineWidthCoef, dimensions.y * gridlineWidthCoef, dimensions.z));
		gridLinesZGfx->shininess = 6.0;
		gridLinesZGfx->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);
		gridLinesZGfx->drawable->updateActiveInstanceParams();
	}
}

SimulationGfx3D::SimulationGfx3D(std::shared_ptr<renderer::RenderEngine> engine, std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager,
	glm::ivec2 screenStart, glm::ivec2 screenSize, unsigned int maxParticleNum)
	: engine(engine), simulationManager(simulationManager)
{
	this->screenStart = screenStart;
	this->screenSize = screenSize;
	shaderProgramTextured = std::make_shared<renderer::ShaderProgram>("shaders/3D_object.vs", "shaders/3D_object_textured.fs", engine);
	shaderProgramNotTextured = std::make_shared<renderer::ShaderProgram>("shaders/3D_object.vs", "shaders/3D_object_not_textured.fs", engine);
	shaderProgramNotTexturedArray = std::make_shared<renderer::ShaderProgram>("shaders/3D_objectArray.vs", "shaders/3D_objectArray.fs", engine);

	glm::dvec3 dim = simulationManager->getDimensions();
	modelRotationPoint = glm::vec3(dim.x, dim.y, dim.z) * 0.5f;

	camera = std::make_unique<renderer::Camera3D>(glm::vec3(10, 10, 0), 40, float(screenSize.x) / screenSize.y);
	camera->addProgram({ shaderProgramTextured, shaderProgramNotTextured, shaderProgramNotTexturedArray });
	camera->rotateAroundPoint(modelRotationPoint, cameraDistance, -20, 40);

	lights = std::make_shared<renderer::Lights>();
	lights->addProgram({ shaderProgramTextured, shaderProgramNotTextured, shaderProgramNotTexturedArray });
	lights->lights.push_back(std::make_unique<renderer::Light>(glm::vec4(50, 50, 50, 1), glm::vec3(1000, 2000, 2000)));
	lights->lights.push_back(std::make_unique<renderer::Light>(glm::vec4(50, 50, -50, 1), glm::vec3(2000, 1000, 2000)));
	lights->lights.push_back(std::make_unique<renderer::Light>(glm::vec4(-50, 50, -50, 1), glm::vec3(2000, 2000, 1000)));
	lights->lights.push_back(std::make_unique<renderer::Light>(glm::vec4(-50, 50, 50, 1), glm::vec3(2000, 2000, 2000)));
	lights->setUniformsForAllPrograms();

	keyCallbackId = engine->keyCallback.onCallback([this](int a, int b, int c, int d) { keyCallback(a, b, c, d); });
	mouseCallbackId = engine->mouseCallback.onCallback([this](double a, double b) { mouseCallback(a, b); });
	mouseButtonCallbackId = engine->mouseButtonCallback.onCallback([this](int a, int b, int c) { mouseButtonCallback(a, b, c); });
	scrollCallbackId = engine->scrollCallback.onCallback([this](double a, double b) { scrollCallback(a, b); });

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

	prevGridlineColor = gridLineColor.value;
	prevCellD = simulationManager->getCellD();
	initGridLines();

	showShaderProgram = std::make_shared<renderer::ShaderProgram>("shaders/quad.vs", "shaders/quad.fs", engine);
	showSquare = std::make_shared<renderer::Square>();
	renderTargetTexture = std::make_shared<renderer::RenderTargetTexture>(screenSize.x, screenSize.y, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	(*showShaderProgram)["colorTexture"] = *renderTargetTexture;
	std::vector<std::shared_ptr<renderer::RenderTargetTexture>> textures = { renderTargetTexture };
	std::shared_ptr<renderer::RenderTargetTexture> renderTargetDepthTexture = std::make_shared<renderer::RenderTargetTexture>
		(screenSize.x, screenSize.y, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);
	renderTargetFramebuffer = std::make_shared<renderer::Framebuffer>(textures, renderTargetDepthTexture, false);

	fluidSurfaceGfx = std::make_unique<FluidSurfaceGfx>(engine, renderTargetFramebuffer, simulationManager, camera, lights, maxParticleNum);

	addParamLine(ParamLine({ &fluidSurfaceEnabled }));
}

void SimulationGfx3D::setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> manager)
{
	obstacleHitboxes.clear();
	obstacleGfxArray.clear();
	lastSelectedObstacle = -1;
	selectedObstacle = -1;
	simulationManager = manager;
	fluidSurfaceGfx->setNewSimulationManager(manager);
}

void SimulationGfx3D::handleTimePassed(double dt)
{
	if (!mouseValid)
		return;
	glm::vec3 forward = camera->getFrontVec();
	forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));
	glm::vec3 right = camera->getRightVec();
	const float movementSpeed = dt * 20.0f;
	if (keys[GLFW_KEY_W])
		camera->move(forward * movementSpeed);
	if (keys[GLFW_KEY_A])
		camera->move(-right * movementSpeed);
	if (keys[GLFW_KEY_S])
		camera->move(-forward * movementSpeed);
	if (keys[GLFW_KEY_D])
		camera->move(right * movementSpeed);
	if (keys[GLFW_KEY_SPACE])
		camera->move(glm::vec3(0, 1, 0) * movementSpeed);
	if (keys[GLFW_KEY_LEFT_SHIFT])
		camera->move(glm::vec3(0, -1, 0) * movementSpeed);
}

void SimulationGfx3D::addObstacle(std::unique_ptr<renderer::Object3D<renderer::Geometry>> obstacleGfx, std::unique_ptr<genericfsim::manager::Obstacle> obstacle, glm::dvec3 size)
{
	glm::dvec3 center = simulationManager->getDimensions() * 0.5;

	obstacleGfx->setPosition(glm::vec4(center, 1));
	obstacleGfx->shininess = 80;
	obstacleGfx->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);
	this->obstacleGfxArray.push_back(std::move(obstacleGfx));
	obstacleHitboxes.push_back({ center, size });

	auto obstacles = simulationManager->getObstacles();
	obstacle->pos = center;
	obstacles.push_back(std::move(obstacle));
	simulationManager->setObstacles(std::move(obstacles));
}

void SimulationGfx3D::addSphericalObstacle(glm::vec3 color, float r)
{
	auto obj = std::make_unique<renderer::Object3D<renderer::Geometry>>(std::make_shared<renderer::Sphere>(80), shaderProgramNotTextured);
	obj->setScale(glm::vec3(r, r, r));
	obj->diffuseColor = glm::vec4(color, 1);
	addObstacle(std::move(obj), std::make_unique<genericfsim::obstacle::SphericalObstacle>(r), glm::vec3(r * 2, r * 2, r * 2));
}

void SimulationGfx3D::addParticleSource(glm::vec3 color, float r, float particleSpawnRate, float particleSpawnSpeed)
{
	auto obj = std::make_unique<renderer::Object3D<renderer::Geometry>>(std::make_shared<renderer::Sphere>(80), shaderProgramNotTextured);
	obj->setScale(glm::vec3(r, r, r));
	obj->diffuseColor = glm::vec4(color, 1);
	addObstacle(std::move(obj), std::make_unique<genericfsim::obstacle::SphericalParticleSource>(r, particleSpawnRate, particleSpawnSpeed), glm::vec3(r * 2, r * 2, r * 2));
}

void SimulationGfx3D::addParticleSink(glm::vec3 color, float r)
{
	auto obj = std::make_unique<renderer::Object3D<renderer::Geometry>>(std::make_shared<renderer::Sphere>(80), shaderProgramNotTextured);
	obj->setScale(glm::vec3(r, r, r));
	obj->diffuseColor = glm::vec4(color, 1);
	addObstacle(std::move(obj), std::make_unique<genericfsim::obstacle::SphericalParticleSink>(r), glm::vec3(r * 2, r * 2, r * 2));
}

void SimulationGfx3D::addRectengularObstacle(glm::vec3 color, glm::vec3 size)
{
	auto obj = std::make_unique<renderer::Object3D<renderer::Geometry>>(std::make_shared<renderer::Cube>(), shaderProgramNotTextured);
	obj->setScale(size);
	obj->diffuseColor = glm::vec4(color, 1);
	addObstacle(std::move(obj), std::make_unique<genericfsim::obstacle::RectengularObstacle>(size), size);
}

void SimulationGfx3D::removeObstacle()
{
	if (lastSelectedObstacle == -1)
		lastSelectedObstacle = obstacleGfxArray.size() - 1;
	if (lastSelectedObstacle == -1)
		return;
	auto obstacles = simulationManager->getObstacles();
	obstacles.erase(obstacles.begin() + lastSelectedObstacle);
	simulationManager->setObstacles(std::move(obstacles));
	obstacleGfxArray.erase(obstacleGfxArray.begin() + lastSelectedObstacle);
	obstacleHitboxes.erase(obstacleHitboxes.begin() + lastSelectedObstacle);
	lastSelectedObstacle = -1;
}

void SimulationGfx3D::show(int screenWidth)
{
	ParamLineCollection::show(screenWidth);
	if (fluidSurfaceEnabled.value)
	{
		fluidSurfaceGfx->show(screenWidth);
	}
}

static glm::dvec3 findIntersection(const glm::dvec3& planePoint, const glm::dvec3& planeNormal, const glm::dvec3& linePoint, const glm::dvec3& lineDirection)
{
	double dotNumerator = glm::dot(planeNormal, (planePoint - linePoint));
	double dotDenominator = glm::dot(planeNormal, lineDirection);
	if (glm::abs(dotDenominator) < 1e-6)
	{
		return glm::dvec3(0, 0, 0);
	}
	double t = dotNumerator / dotDenominator;
	return linePoint + t * lineDirection;
}

void SimulationGfx3D::selectObstacle()
{
	const glm::vec3 rayDirTmp = camera->getMouseRayDir(mousePos);
	const glm::dvec3 rayDirection = glm::dvec3(rayDirTmp.x, rayDirTmp.y, rayDirTmp.z);
	const glm::dvec3 camPos = camera->getPosition();

	double closestPoint = 1e8;
	selectedObstacle = -1;
	for (auto i = 0; i < obstacleHitboxes.size(); i++)
	{
		const glm::dvec3 center = obstacleHitboxes[i].center;
		const glm::dvec3 size = obstacleHitboxes[i].size;
		for (int axis = 0; axis < 3; axis++)
		{
			for (int sign = -1; sign <= 1; sign += 2)
			{
				glm::dvec3 planePoint = center;
				planePoint[axis] += sign * size[axis] * 0.5;
				glm::dvec3 planeNormal(0, 0, 0);
				planeNormal[axis] = sign;
				glm::dvec3 intersection = findIntersection(planePoint, planeNormal, camPos, rayDirection);
				glm::ivec3 ignoreAxis(0, 0, 0);
				ignoreAxis[axis] = 1;
				if ((intersection.x >= center.x - size.x * 0.5 && intersection.x <= center.x + size.x * 0.5 || ignoreAxis.x)
					&& (intersection.y >= center.y - size.y * 0.5 && intersection.y <= center.y + size.y * 0.5 || ignoreAxis.y)
					&& (intersection.z >= center.z - size.z * 0.5 && intersection.z <= center.z + size.z * 0.5 || ignoreAxis.z))
				{
					double dist = glm::length(intersection - camPos);
					if (dist < closestPoint)
					{
						closestPoint = dist;
						selectedObstacle = i;
						lastSelectedObstacle = i;
					}
				}
			}
		}
	}
}

void SimulationGfx3D::handleObstacleMovement()
{
	if (selectedObstacle == -1)
		return;
	glm::dvec3 obstacleCenter = obstacleHitboxes[selectedObstacle].center;
	glm::dvec3 camDir = glm::normalize(camera->getFrontVec());
	glm::dvec3 camPos = camera->getPosition();
	const glm::vec3 prevRayDirTmp = camera->getMouseRayDir(prevMousePos);
	glm::dvec3 prevRayDirection = glm::dvec3(prevRayDirTmp.x, prevRayDirTmp.y, prevRayDirTmp.z);
	const glm::vec3 rayDirTmp = camera->getMouseRayDir(mousePos);
	glm::dvec3 rayDirection = glm::dvec3(rayDirTmp.x, rayDirTmp.y, rayDirTmp.z);
	glm::dvec3 prevIntersection = findIntersection(obstacleCenter, camDir, camPos, prevRayDirection);
	glm::dvec3 intersection = findIntersection(obstacleCenter, camDir, camPos, rayDirection);
	glm::dvec3 movement = intersection - prevIntersection;
	obstacleHitboxes[selectedObstacle].center += movement;
	obstacleGfxArray[selectedObstacle]->setPosition(glm::vec4(glm::toVec3(obstacleHitboxes[selectedObstacle].center), 1));
	auto obstacles = simulationManager->getObstacles();
	obstacles[selectedObstacle]->setNewPos(obstacleHitboxes[selectedObstacle].center);
	simulationManager->setObstacles(std::move(obstacles));
}

void SimulationGfx3D::drawGridLines()
{
	if (prevCellD != simulationManager->getCellD())
	{
		prevCellD = simulationManager->getCellD();
		initGridLines();
	}
	else if (prevGridlineColor != gridLineColor.value)
	{
		prevGridlineColor = gridLineColor.value;
		auto linesX = gridLinesXGfx->drawable;
		for (int i = 0; i < linesX->getActiveInstanceNum(); i++)
		{
			linesX->setColor(i, glm::vec4(gridLineColor.value, 1));
		}
		linesX->updateActiveInstanceParams();
		auto linesY = gridLinesYGfx->drawable;
		for (int i = 0; i < linesY->getActiveInstanceNum(); i++)
		{
			linesY->setColor(i, glm::vec4(gridLineColor.value, 1));
		}
		linesY->updateActiveInstanceParams();
		auto linesZ = gridLinesZGfx->drawable;
		for (int i = 0; i < linesZ->getActiveInstanceNum(); i++)
		{
			linesZ->setColor(i, glm::vec4(gridLineColor.value, 1));
		}
		linesZ->updateActiveInstanceParams();
	}
	gridLinesXGfx->draw();
	gridLinesYGfx->draw();
	gridLinesZGfx->draw();
}

void SimulationGfx3D::render()
{
	if (prevScreenStart != screenStart || prevScreenSize != screenSize)
	{
		prevScreenStart = screenStart;
		prevScreenSize = screenSize;
		handleScreenChanged();
	}

	renderTargetFramebuffer->bind();

	glm::dvec3 dim = simulationManager->getDimensions();
	glm::vec3 size = glm::vec3(dim.x, dim.y, dim.z);
	glm::vec3 center = size * 0.5f;
	modelRotationPoint = center;

	engine->enableDepthTest(true);
	engine->setViewport(0, 0, screenSize.x, screenSize.y);
	engine->clearViewport(glm::vec4(0.1, 0, 0, 0), 1.0f);

	planeGfx->setPosition(glm::vec4(center.x, -0.05f, center.z, 1));
	planeGfx->draw();

	for (auto& obstacle : obstacleGfxArray)
	{
		obstacle->draw();
	}

	if (gridlinesEnabled.value)
		drawGridLines();

	transparentBox->draw(center, size, true, false);

	if (fluidSurfaceEnabled.value)
	{
		fluidSurfaceGfx->render();
		engine->enableDepthTest(true);
		engine->setViewport(0, 0, screenSize.x, screenSize.y);
		renderTargetFramebuffer->bind();
	}
	else
		drawParticles();

	transparentBox->draw(center, size, false, true);

	engine->bindDefaultFramebuffer();
	engine->setViewport(screenStart.x, engine->getScreenHeight() - (screenStart.y + screenSize.y), screenSize.x, screenSize.y);
	engine->clearViewport(glm::vec4(0.1, 0, 0, 0), 1.0);
	showShaderProgram->activate();
	engine->enableDepthTest(false);
	showSquare->draw();
	engine->enableDepthTest(true);
}

SimulationGfx3D::~SimulationGfx3D()
{
	spdlog::debug("SimulationGfx3D destructor");
	engine->mouseCallback.removeCallbackFunction(mouseCallbackId);
	engine->mouseButtonCallback.removeCallbackFunction(mouseButtonCallbackId);
	engine->scrollCallback.removeCallbackFunction(scrollCallbackId);
	engine->keyCallback.removeCallbackFunction(keyCallbackId);
}
