#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <engineUtils/lights.hpp>
#include <engineUtils/camera3D.hpp>
#include <engineUtils/object.h>
#include <geometries/basicGeometries.h>
#include <engine/renderEngine.h>
#include <engine/texture.h>
#include <engine/shaderProgram.h>
#include <engine/shaderProgram.h>
#include "manager/simulationManager.h"
#include "../gfxInterface.hpp"
#include <vector>
#include "transparentBox.hpp"


class SimulationGfx3D : public GfxInterface {
private:
	std::shared_ptr<renderer::RenderEngine> engine;
	std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager;

	std::shared_ptr<renderer::Camera3D> camera;
	std::shared_ptr<renderer::Lights> lights;

	std::unique_ptr<renderer::Object3D<renderer::Geometry>> planeGfx;
	std::unique_ptr<renderer::Object3D<renderer::BasicGeometryArray>> ballsGfx;

	std::unique_ptr<TransparentBox> transparentBox;

	std::vector<std::unique_ptr<renderer::Object3D<renderer::Geometry>>> obstacleGfxArray;

	std::unique_ptr<renderer::Object3D<renderer::BasicGeometryArray>> gridLinesXGfx;
	std::unique_ptr<renderer::Object3D<renderer::BasicGeometryArray>> gridLinesYGfx;
	std::unique_ptr<renderer::Object3D<renderer::BasicGeometryArray>> gridLinesZGfx;

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
	
	std::shared_ptr<renderer::ShaderProgram> shaderProgramTextured;
	std::shared_ptr<renderer::ShaderProgram> shaderProgramNotTextured;
	std::shared_ptr<renderer::ShaderProgram> shaderProgramNotTexturedArray;

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

	void selectObstacle();
	void handleObstacleMovement();

	bool particleSpeedColorWasEnabled = false;
	void drawParticles();

	void initGridLines();
	void drawGridLines();

	void addObstacle(std::unique_ptr<renderer::Object3D<renderer::Geometry>> obstacleGfx, std::unique_ptr<genericfsim::manager::Obstacle> obstacle, glm::dvec3 size);

public:
	SimulationGfx3D(std::shared_ptr<renderer::RenderEngine> engine, std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager, 
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

