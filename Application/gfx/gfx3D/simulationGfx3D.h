#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <engineUtils/lights.hpp>
#include <engineUtils/camera3D.hpp>
#include <engineUtils/object3D.hpp>
#include "geometries/quad.h"
#include "manager/simulationManager.h"
#include "../gfxInterface.hpp"
#include <vector>
#include "transparentBox.hpp"


class SimulationGfx3D : public GfxInterface {
private:
	std::shared_ptr<RenderEngine> engine;
	std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager;

	std::unique_ptr<Camera3D> camera;
	std::unique_ptr<Lights> lights;
	unsigned int floorTexture;
	std::unique_ptr<Object3D> plane;
	std::unique_ptr<Object3D> balls;

	std::unique_ptr<TransparentBox> transparentBox;

	std::vector<std::unique_ptr<Object3D>> obstacleGfx;

	std::unique_ptr<Object3D> gridLinesX;
	std::unique_ptr<Object3D> gridLinesY;
	std::unique_ptr<Object3D> gridLinesZ;

	struct Hitbox {
		glm::dvec3 center;
		glm::dvec3 size;
	};
	std::vector<Hitbox> obstacleHitboxes;
	int selectedObstacle = -1;
	int lastSelectedObstacle = -1;

	glm::vec3 prevColor;
	unsigned int prevParticleNum;

	glm::vec3 prevGridlineColor;
	glm::dvec3 prevCellD;
	
	unsigned int shaderProgramTextured;
	unsigned int shaderProgramNotTextured;

	int mouseCallbackId;
	int mouseButtonCallbackId;
	int scrollCallbackId;
	int keyCallbackId;
	bool keys[1024] = { 0 };

	glm::vec2 mousePos = glm::vec2(0.5, 0.5);
	glm::vec2 prevMousePos = glm::vec2(0.5, 0.5);
	bool isLeftButtonDown = false;
	bool isMiddleButtonDown = false;
	bool isRightButtonDown = false;
	bool mouseValid = false;

	float cameraDistance = 60;
	const float minCameraDistance = 1;
	const float maxCameraDIstance = 200;
	glm::vec3 modelRotationPoint = glm::vec3(0, 0, 0);
	bool inModelRotationMode = false;
	
	void mouseCallback(double x, double y);
	void mouseButtonCallback(int button, int action, int mods);
	void scrollCallback(double xoffset, double yoffset);
	void keyCallback(int key, int scancode, int action, int mode);

	glm::dvec3 getMouseRayDirection(glm::vec2 mousePos);
	void selectObstacle();
	void handleObstacleMovement();

	bool particleSpeedColorWasEnabled = false;
	void drawParticles();

	void initGridLines();
	void drawGridLines();

	void addObstacle(std::shared_ptr<Geometry> obstacleGfx, std::unique_ptr<genericfsim::manager::Obstacle> obstacle, glm::dvec3 size);

public:
	SimulationGfx3D(std::shared_ptr<RenderEngine> engine, std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager, 
						glm::ivec2 screenStart, glm::ivec2 screenSize, unsigned int maxParticleNum);
	
	void setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> manager) override;

	void handleTimePassed(double dt) override;

	void render() override;

	void addSphericalObstacle(glm::vec3 color, float r) override;

	void addRectengularObstacle(glm::vec3 color, glm::vec3 size) override;

	void addParticleSource(glm::vec3 color, float r, float particleSpawnRate, float particleSpawnSpeed);

	void addParticleSink(glm::vec3 color, float r);

	void removeObstacle() override;

	~SimulationGfx3D() override;

};

