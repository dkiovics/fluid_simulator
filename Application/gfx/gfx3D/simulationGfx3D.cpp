#include "simulationGfx3D.h"
#include <algorithm>
#include <geometries/rectangle.h>
#include <geometries/quad.h>
#include <geometries/sphere.h>
#include <map>

void SimulationGfx3D::mouseCallback(double x, double y) {
	int totalH = engine->getScreenHeight();
	mousePos = glm::vec2((x - screenStart.x) / screenSize.x, ((totalH - y) - screenStart.y) / screenSize.y);
	if (mousePos.x < 0 || mousePos.x > 1 || mousePos.y < 0 || mousePos.y > 1) {
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
	if (isLeftButtonDown) {
		camera->incrementPitchAndYaw(d.y, d.x);
	}
	else if (isMiddleButtonDown) {
		camera->rotateAroundPoint(modelRotationPoint, cameraDistance, d.y, d.x);
	}
	else if (isRightButtonDown) {
		handleObstacleMovement();
	}
	prevMousePos = mousePos;
}

void SimulationGfx3D::mouseButtonCallback(int button, int action, int mods) {
	if (!mouseValid)
		return;
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			isLeftButtonDown = true;
			inModelRotationMode = false;
		}
		else if (action == GLFW_RELEASE)
			isLeftButtonDown = false;
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
		if (action == GLFW_PRESS) {
			isMiddleButtonDown = true;
			inModelRotationMode = true;
			camera->rotateAroundPoint(modelRotationPoint, cameraDistance, 0, 0);
		}
		else if (action == GLFW_RELEASE)
			isMiddleButtonDown = false;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) {
			isRightButtonDown = true;
			selectObstacle();
		}
		else if (action == GLFW_RELEASE) {
			isRightButtonDown = false;
			selectedObstacle = -1;
		}
	}
}

void SimulationGfx3D::scrollCallback(double xoffset, double yoffset) {
	if (!mouseValid)
		return;
	if (inModelRotationMode) {
		if (yoffset < 0) {
			cameraDistance = std::clamp(cameraDistance + 5.0f, minCameraDistance, maxCameraDIstance);
		}
		else if (yoffset > 0) {
			cameraDistance = std::clamp(cameraDistance - 5.0f, minCameraDistance, maxCameraDIstance);
		}
		camera->rotateAroundPoint(modelRotationPoint, cameraDistance, 0, 0);
	}
}

void SimulationGfx3D::keyCallback(int key, int scancode, int action, int mode) {
	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS) {
			keys[key] = true;
			inModelRotationMode = false;
		}
		else if (action == GLFW_RELEASE) {
			keys[key] = false;
		}
	}
}

void SimulationGfx3D::drawParticles() {
	auto particles = simulationManager->getParticleGfxData();
	auto& geometry = *balls->geometry;
	const int particleNum = particles.size();
	const float maxSpeedInv = 1.0f / maxParticleSpeed;
#pragma omp parallel for
	for (int p = 0; p < particleNum; p++) {
		geometry.positionAt(p) = particles[p].pos;
		if (particleSpeedColorEnabled) {
			float s = std::min(particles[p].v * maxSpeedInv, 1.0f);
			s = std::pow(s, 0.3f);
			geometry.colorAt(p) = glm::vec4((particleColor * (1.0f - s)) + (particleSpeedColor * s), 1);
		}
	}
	geometry.updatePosition(particleNum);
	if (particleSpeedColorEnabled) {
		geometry.updateColor(particleNum);
		particleSpeedColorWasEnabled = true;
	}

	if (!particleSpeedColorEnabled && (particleSpeedColorWasEnabled || prevColor != particleColor || particleNum != prevParticleNum)) {
		prevParticleNum = particleNum;
		particleSpeedColorWasEnabled = false;
		prevColor = particleColor;
		for (int p = 0; p < particleNum; p++) {
			geometry.colorAt(p) = glm::vec4(particleColor, 1);
		}
		geometry.updateColor(particleNum);
	}
	float r = simulationManager->getConfig().particleRadius;
	balls->M = glm::scale(glm::mat4(1.0), glm::vec3(r, r, r));
	balls->draw(particleNum);
}

void SimulationGfx3D::initGridLines() {
	constexpr double gridlineWidthCoef = 0.0004;
	const glm::ivec3 gridSize = simulationManager->getGridSize();
	const glm::dvec3 cellD = simulationManager->getCellD();
	const glm::dvec3 dimensions = simulationManager->getDimensions();
	{
		std::vector<glm::vec3> linesXPos;
		std::vector<glm::vec4> color;
		for (int y = 1; y < gridSize.y - 1; y++) {
			for (int z = 1; z < gridSize.z - 1; z++) {
				linesXPos.push_back(glm::vec3(dimensions.x * 0.5, y * cellD.y, z * cellD.z));
				color.push_back(glm::vec4(gridLineColor, 1));
			}
		}
		std::shared_ptr<Rectangle> linesX = std::make_shared<Rectangle>(linesXPos.size(), linesXPos, color, engine, dimensions.x, dimensions.y * gridlineWidthCoef, dimensions.z * gridlineWidthCoef);
		gridLinesX = std::make_unique<Object3D>(linesX, shaderProgramNotTextured, glm::mat4(1.0f), glm::vec3(1.2, 1.2, 1.2), 6.0);
	}
{
		std::vector<glm::vec3> linesYPos;
		std::vector<glm::vec4> color;
		for (int x = 1; x < gridSize.x - 1; x++) {
			for (int z = 1; z < gridSize.z - 1; z++) {
				linesYPos.push_back(glm::vec3(x * cellD.x, dimensions.y * 0.5, z * cellD.z));
				color.push_back(glm::vec4(gridLineColor, 1));
			}
		}
		std::shared_ptr<Rectangle> linesY = std::make_shared<Rectangle>(linesYPos.size(), linesYPos, color, engine, dimensions.x * gridlineWidthCoef, dimensions.y, dimensions.z * gridlineWidthCoef);
		gridLinesY = std::make_unique<Object3D>(linesY, shaderProgramNotTextured, glm::mat4(1.0f), glm::vec3(1.2, 1.2, 1.2), 6.0);
	}

	{
		std::vector<glm::vec3> linesZPos;
		std::vector<glm::vec4> color;
		for (int x = 1; x < gridSize.x - 1; x++) {
			for (int y = 1; y < gridSize.y - 1; y++) {
				linesZPos.push_back(glm::vec3(x * cellD.x, y * cellD.y, dimensions.z * 0.5));
				color.push_back(glm::vec4(gridLineColor, 1));
			}
		}
		std::shared_ptr<Rectangle> linesZ = std::make_shared<Rectangle>(linesZPos.size(), linesZPos, color, engine, dimensions.x * gridlineWidthCoef, dimensions.y * gridlineWidthCoef, dimensions.z);
		gridLinesZ = std::make_unique<Object3D>(linesZ, shaderProgramNotTextured, glm::mat4(1.0f), glm::vec3(1.2, 1.2, 1.2), 6.0);
	}
}

SimulationGfx3D::SimulationGfx3D(std::shared_ptr<renderer::RenderEngine> engine, std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager,
									glm::ivec2 screenStart, glm::ivec2 screenSize, unsigned int maxParticleNum)
										: engine(engine), simulationManager(simulationManager) {
	this->screenStart = screenStart;
	this->screenSize = screenSize;
	shaderProgramTextured = engine->createGPUProgram("shaders/3D_objects.vs", "shaders/3D_objects_textured.fs");
	shaderProgramNotTextured = engine->createGPUProgram("shaders/3D_objects.vs", "shaders/3D_objects_not_textured.fs");

	glm::dvec3 dim = simulationManager->getDimensions();
	modelRotationPoint = glm::vec3(dim.x, dim.y, dim.z) * 0.5f;
	
	camera = std::make_unique<Camera3D>(engine, std::vector<unsigned int>{ shaderProgramTextured, shaderProgramNotTextured }, glm::vec3(10, 10, 0), 40, float(screenSize.x) / screenSize.y);
	camera->rotateAroundPoint(modelRotationPoint, cameraDistance, -20, 40);
	keyCallbackId = engine->keyCallback.onCallback([this](int a, int b, int c, int d) { keyCallback(a, b, c, d); });
	mouseCallbackId = engine->mouseCallback.onCallback([this](double a, double b) { mouseCallback(a, b); });
	mouseButtonCallbackId = engine->mouseButtonCallback.onCallback([this](int a, int b, int c) { mouseButtonCallback(a, b, c); });
	scrollCallbackId = engine->scrollCallback.onCallback([this](double a, double b) { scrollCallback(a, b); });

	floorTexture = engine->loadTexture("shaders/tiles.jpg", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
	std::shared_ptr<Quad> floor = std::make_shared<Quad>(1, std::vector<glm::vec3>{ glm::vec3(0, -0.1, 0) }, std::vector<glm::vec4>{ glm::vec4(1, 1, 1, 1) }, engine, 200, 200);
	glm::mat4 planeMatrix = glm::rotate(glm::mat4(1.0f), glm::pi<float>() * 0.5f, glm::vec3(1, 0, 0));
	plane = std::make_unique<Object3D>(floor, shaderProgramTextured, planeMatrix, glm::vec3(0.7, 0.7, 0.7), 5.8, true, floorTexture, 5);
	
	transparentBox = std::make_unique<TransparentBox>(*camera, engine, glm::vec4(0.8, 0.5, 0.2, 0.4), shaderProgramNotTextured);

	std::vector<glm::vec3> positions(maxParticleNum);
	std::vector<glm::vec4> colors(maxParticleNum);
	std::shared_ptr<Sphere> spheres = std::make_shared<Sphere>(maxParticleNum, positions, colors, engine, 1, 8);
	balls = std::make_unique<Object3D>(spheres, shaderProgramNotTextured, glm::mat4(1.0f), glm::vec3(1.2, 1.2, 1.2), 80/*, true, engine->loadTexture("shaders/doge.jpg", GL_LINEAR, GL_LINEAR)*/);

	lights = std::make_unique<Lights>(engine, std::vector<unsigned int>{ shaderProgramTextured, shaderProgramNotTextured });
	lights->lights.push_back(Light(glm::vec4(50, 50, 50, 1), glm::vec3(1000, 2000, 2000)));
	lights->lights.push_back(Light(glm::vec4(50, 50, -50, 1), glm::vec3(2000, 1000, 2000)));
	lights->lights.push_back(Light(glm::vec4(-50, 50, -50, 1), glm::vec3(2000, 2000, 1000)));
	lights->lights.push_back(Light(glm::vec4(-50, 50, 50, 1), glm::vec3(2000, 2000, 2000)));

	prevGridlineColor = gridLineColor;
	prevCellD = simulationManager->getCellD();
	initGridLines();
}

void SimulationGfx3D::setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> manager) {
	obstacleHitboxes.clear();
	obstacleGfx.clear();
	lastSelectedObstacle = -1;
	selectedObstacle = -1;
	simulationManager = manager;
}

void SimulationGfx3D::handleTimePassed(double dt) {
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

void SimulationGfx3D::addObstacle(std::shared_ptr<Geometry> obstacleGfx, std::unique_ptr<genericfsim::manager::Obstacle> obstacle, glm::dvec3 size) {
	glm::dvec3 center = simulationManager->getDimensions() * 0.5;

	this->obstacleGfx.push_back(std::make_unique<Object3D>(obstacleGfx, shaderProgramNotTextured, glm::translate(glm::mat4(1), glm::toVec3(center)), glm::vec3(1.2, 1.2, 1.2), 80));
	obstacleHitboxes.push_back({ center, size });

	auto obstacles = simulationManager->getObstacles();
	obstacle->pos = center;
	obstacles.push_back(std::move(obstacle));
	simulationManager->setObstacles(std::move(obstacles));
}

void SimulationGfx3D::addSphericalObstacle(glm::vec3 color, float r) {
	addObstacle(std::make_shared<Sphere>(1, std::vector<glm::vec3>{ glm::vec3(0, 0, 0) }, std::vector<glm::vec4>{ glm::vec4(color, 1) }, engine, r, 80),
				std::make_unique<genericfsim::obstacle::SphericalObstacle>(r), glm::vec3(r * 2, r * 2, r * 2));
}

void SimulationGfx3D::addParticleSource(glm::vec3 color, float r, float particleSpawnRate, float particleSpawnSpeed) {
	addObstacle(std::make_shared<Sphere>(1, std::vector<glm::vec3>{ glm::vec3(0, 0, 0) }, std::vector<glm::vec4>{ glm::vec4(color, 1) }, engine, r, 80),
						std::make_unique<genericfsim::obstacle::SphericalParticleSource>(r, particleSpawnRate, particleSpawnSpeed), glm::vec3(r * 2, r * 2, r * 2));
}

void SimulationGfx3D::addParticleSink(glm::vec3 color, float r) {
	addObstacle(std::make_shared<Sphere>(1, std::vector<glm::vec3>{ glm::vec3(0, 0, 0) }, std::vector<glm::vec4>{ glm::vec4(color, 1) }, engine, r, 80),
		std::make_unique<genericfsim::obstacle::SphericalParticleSink>(r), glm::vec3(r * 2, r * 2, r * 2));
}

void SimulationGfx3D::addRectengularObstacle(glm::vec3 color, glm::vec3 size) {
	addObstacle(std::make_shared<Rectangle>(1, std::vector<glm::vec3>{ glm::vec3(0, 0, 0) }, std::vector<glm::vec4>{ glm::vec4(color, 1) }, engine, size.x, size.y, size.z),
				std::make_unique<genericfsim::obstacle::RectengularObstacle>(size), size);
}

void SimulationGfx3D::removeObstacle() {
	if (lastSelectedObstacle == -1)
		lastSelectedObstacle = obstacleGfx.size() - 1;
	if (lastSelectedObstacle == -1)
		return;
	auto obstacles = simulationManager->getObstacles();
	obstacles.erase(obstacles.begin() + lastSelectedObstacle);
	simulationManager->setObstacles(std::move(obstacles));
	obstacleGfx.erase(obstacleGfx.begin() + lastSelectedObstacle);
	obstacleHitboxes.erase(obstacleHitboxes.begin() + lastSelectedObstacle);
	lastSelectedObstacle = -1;
}

glm::dvec3 SimulationGfx3D::getMouseRayDirection(glm::vec2 mousePos) {
	glm::vec4 viewport = glm::vec4(0, 0, screenSize.x, screenSize.y);
	glm::vec3 winCoords = glm::vec3(mousePos.x * screenSize.x, mousePos.y * screenSize.y, 0.0f);
	glm::vec3 nearPoint = glm::unProject(winCoords, camera->getViewMatrix(), camera->getProjectionMatrix(), viewport);
	winCoords.z = 1.0;
	glm::vec3 farPoint = glm::unProject(winCoords, camera->getViewMatrix(), camera->getProjectionMatrix(), viewport);
	glm::vec3 rayDirection = glm::normalize(farPoint - nearPoint);
	return glm::dvec3(rayDirection.x, rayDirection.y, rayDirection.z);
}

glm::dvec3 findIntersection(const glm::dvec3& planePoint, const glm::dvec3& planeNormal, const glm::dvec3& linePoint, const glm::dvec3& lineDirection) {
	double dotNumerator = glm::dot(planeNormal, (planePoint - linePoint));
	double dotDenominator = glm::dot(planeNormal, lineDirection);
	if (glm::abs(dotDenominator) < 1e-6) {
		return glm::dvec3(0, 0, 0);
	}
	double t = dotNumerator / dotDenominator;
	return linePoint + t * lineDirection;
}

void SimulationGfx3D::selectObstacle() {
	const glm::dvec3 rayDirection = getMouseRayDirection(mousePos);
	const glm::dvec3 camPos = camera->getPosition();
	
	double closestPoint = 1e8;
	selectedObstacle = -1;
	for (auto i = 0; i < obstacleHitboxes.size(); i++) {
		const glm::dvec3 center = obstacleHitboxes[i].center;
		const glm::dvec3 size = obstacleHitboxes[i].size;
		for (int axis = 0; axis < 3; axis++) {
			for (int sign = -1; sign <= 1; sign += 2) {
				glm::dvec3 planePoint = center;
				planePoint[axis] += sign * size[axis] * 0.5;
				glm::dvec3 planeNormal(0, 0, 0);
				planeNormal[axis] = sign;
				glm::dvec3 intersection = findIntersection(planePoint, planeNormal, camPos, rayDirection);
				glm::ivec3 ignoreAxis(0, 0, 0);
				ignoreAxis[axis] = 1;
				if ((intersection.x >= center.x - size.x * 0.5 && intersection.x <= center.x + size.x * 0.5 || ignoreAxis.x)
					&& (intersection.y >= center.y - size.y * 0.5 && intersection.y <= center.y + size.y * 0.5 || ignoreAxis.y)
					&& (intersection.z >= center.z - size.z * 0.5 && intersection.z <= center.z + size.z * 0.5 || ignoreAxis.z)) {
					double dist = glm::length(intersection - camPos);
					if (dist < closestPoint) {
						closestPoint = dist;
						selectedObstacle = i;
						lastSelectedObstacle = i;
					}
				}
			}
		}
	}
}

void SimulationGfx3D::handleObstacleMovement() {
	if (selectedObstacle == -1)
		return;
	glm::dvec3 obstacleCenter = obstacleHitboxes[selectedObstacle].center;
	glm::dvec3 camDir = glm::normalize(camera->getFrontVec());
	glm::dvec3 camPos = camera->getPosition();
	glm::dvec3 prevRayDirection = getMouseRayDirection(prevMousePos);
	glm::dvec3 rayDirection = getMouseRayDirection(mousePos);
	glm::dvec3 prevIntersection = findIntersection(obstacleCenter, camDir, camPos, prevRayDirection);
	glm::dvec3 intersection = findIntersection(obstacleCenter, camDir, camPos, rayDirection);
	glm::dvec3 movement = intersection - prevIntersection;
	obstacleHitboxes[selectedObstacle].center += movement;
	obstacleGfx[selectedObstacle]->M = glm::translate(glm::mat4(1.0), glm::toVec3(obstacleHitboxes[selectedObstacle].center));
	auto obstacles = simulationManager->getObstacles();
	obstacles[selectedObstacle]->setNewPos(obstacleHitboxes[selectedObstacle].center);
	simulationManager->setObstacles(std::move(obstacles));
}

void SimulationGfx3D::drawGridLines() {
	if (prevCellD != simulationManager->getCellD()) {
		prevCellD = simulationManager->getCellD();
		initGridLines();
	}
	else if (prevGridlineColor != gridLineColor) {
		prevGridlineColor = gridLineColor;
		auto linesX = gridLinesX->geometry;
		for(int i = 0; i < linesX->getInstanceCount(); i++) {
			linesX->colorAt(i) = glm::vec4(gridLineColor, 1);
		}
		linesX->updateColor();
		auto linesY = gridLinesY->geometry;
		for (int i = 0; i < linesY->getInstanceCount(); i++) {
			linesY->colorAt(i) = glm::vec4(gridLineColor, 1);
		}
		linesY->updateColor();
		auto linesZ = gridLinesZ->geometry;
		for (int i = 0; i < linesZ->getInstanceCount(); i++) {
			linesZ->colorAt(i) = glm::vec4(gridLineColor, 1);
		}
		linesZ->updateColor();
	}
	gridLinesX->draw();
	gridLinesY->draw();
	gridLinesZ->draw();
}

void SimulationGfx3D::render() {
	glm::dvec3 dim = simulationManager->getDimensions();
	glm::vec3 size = glm::vec3(dim.x, dim.y, dim.z);
	glm::vec3 center = size * 0.5f;
	modelRotationPoint = center;

	engine->setViewport(screenStart.x, screenStart.y, screenSize.x, screenSize.y);
	glEnable(GL_DEPTH_TEST);
	engine->clearViewport(glm::vec4(0.1, 0, 0, 0), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	camera->setAspectRatio(float(screenSize.x) / screenSize.y);
	camera->setUniforms();
	lights->setUniforms();
	
	plane->M = glm::translate(glm::mat4(1), glm::vec3(center.x, 0, center.z)) * glm::rotate(glm::mat4(1), PI * 0.5f, glm::vec3(1, 0, 0));
	plane->draw();

	drawParticles();

	for (auto& obstacle : obstacleGfx) {
		obstacle->draw();
	}

	if(gridlinesEnabled)
		drawGridLines();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	transparentBox->draw(center, size);
	glDisable(GL_BLEND);
}

SimulationGfx3D::~SimulationGfx3D() {
	engine->mouseCallback.removeCallbackFunction(mouseCallbackId);
	engine->mouseButtonCallback.removeCallbackFunction(mouseButtonCallbackId);
	engine->scrollCallback.removeCallbackFunction(scrollCallbackId);
	engine->keyCallback.removeCallbackFunction(keyCallbackId);
	engine->deleteTexture(floorTexture);
	engine->deleteGPUProgram(shaderProgramTextured);
	engine->deleteGPUProgram(shaderProgramNotTextured);
#ifdef _DEBUG
	std::cout << "SimulationGfx3D destructor called" << std::endl;
#endif
}
