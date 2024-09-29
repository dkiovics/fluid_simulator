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
#include <engine/framebuffer.h>
#include <manager/simulationManager.h>
#include <vector>
#include "renderer3DInterface.h"
#include "gfxInterface/gfxInterface.hpp"


namespace visual
{

class SimulationGfx3DController : public GfxInterface
{
private:
	std::shared_ptr<renderer::RenderEngine> engine;
	std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager;

	std::shared_ptr<renderer::Camera3D> camera;
	std::shared_ptr<renderer::Lights> lights;

	std::vector<std::unique_ptr<renderer::Object3D<renderer::Geometry>>> obstacleGfxArray;

	struct Hitbox
	{
		glm::dvec3 center;
		glm::dvec3 size;
	};
	std::vector<Hitbox> obstacleHitboxes;
	int selectedObstacle = -1;
	int lastSelectedObstacle = -1;

	const int maxParticleNum;

	std::shared_ptr<renderer::GpuProgram> shaderProgramNotTextured;

	std::shared_ptr<renderer::GpuProgram> showShaderProgram;
	std::shared_ptr<renderer::Square> showSquare;
	std::shared_ptr<renderer::RenderTargetTexture> renderTargetTexture;
	std::shared_ptr<renderer::Framebuffer> renderTargetFramebuffer;

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

	std::unique_ptr<Renderer3DInterface> renderer3DInterface;

	ParamBool diffRenderEnabled = ParamBool("Diff Render Enabled", false);

	void mouseCallback(double x, double y);
	void mouseButtonCallback(int button, int action, int mods);
	void scrollCallback(double xoffset, double yoffset);
	void keyCallback(int key, int scancode, int action, int mode);

	void selectObstacle();
	void handleObstacleMovement();

	ConfigData3D getConfigData3D();

	void addObstacle(std::unique_ptr<renderer::Object3D<renderer::Geometry>> obstacleGfx, std::unique_ptr<genericfsim::manager::Obstacle> obstacle, glm::dvec3 size);

public:
	SimulationGfx3DController(std::shared_ptr<renderer::RenderEngine> engine, std::shared_ptr<genericfsim::manager::SimulationManager> simulationManager,
		glm::ivec2 screenStart, glm::ivec2 screenSize, unsigned int maxParticleNum);

	void setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> manager) override;

	void handleTimePassed(double dt) override;

	void render() override;

	void addSphericalObstacle(glm::vec3 color, float r) override;

	void addRectengularObstacle(glm::vec3 color, glm::vec3 size) override;

	void addParticleSource(glm::vec3 color, float r, float particleSpawnRate, float particleSpawnSpeed);

	void addParticleSink(glm::vec3 color, float r);

	void removeObstacle() override;

	void show(int screenWidth) override;

	~SimulationGfx3DController() override;

};

} // namespace visual