#pragma once

#include <engine/windowManager.h>
#include <geometries/basicGeometries.h>
#include <engineUtils/object.h>
#include <manager/simulationManager.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <engineUtils/camera2D.hpp>
#include "gfxInterface.hpp"


namespace visual
{

class SimulationGfx2D : public GfxInterface {
private:
	std::shared_ptr<renderer::WindowManager> engine;
	std::shared_ptr<genericfsim::manager::SimulationManager> simulator;
	const int maxParticleNum;

	int gridWidth;
	int gridHeight;

	glm::vec3 prevParticleColor;

	/*glm::mat4 P;
	float magnification = 1;
	glm::vec3 magPos = glm::vec3(0.5, 0.5, 0);*/
	//In normalized device coordinates
	glm::vec2 mousePos = glm::vec2(0, 0);
	bool mouseValid = true;

	std::shared_ptr<renderer::Camera2D> camera;

	std::shared_ptr<renderer::GpuProgram> basicArrayProgram;
	std::shared_ptr<renderer::GpuProgram> basicArrayProgramSingleColor;
	std::shared_ptr<renderer::GpuProgram> basicProgram;

	std::unique_ptr<renderer::Object2D<renderer::BasicGeometryArray>> gridlinesGfx;
	std::unique_ptr<renderer::Object2D<renderer::BasicGeometryArray>> particlesGfx;

	std::vector<glm::dvec2> obstacleBoundingBoxes;
	std::vector<std::unique_ptr<renderer::Object2D<renderer::Geometry>>> obstacleGfxObjects;

	int draggedObstacle = -1;
	glm::vec2 prevMousePos;

	bool topIsSolid = false;

	int mouseCallbackNum;
	int mouseButtonCallbackNum;
	int scrollCallbackNum;

	bool particleSpeedColorWasEnabled = false;

private:

	void updateParticles();

	void mouseCallback(double x, double y);

	void mouseButtonCallback(int button, int action, int mods);

	void scrollCallback(double xoffset, double yoffset);

	void handleSimSpecChanged();

public:

	SimulationGfx2D(std::shared_ptr<renderer::WindowManager> engine, std::shared_ptr<genericfsim::manager::SimulationManager> simulator, int maxParticleNum);

	//	returns the position on the grid including the magnification, in world coordinates
	glm::vec2 getMouseGridPos();

	void render(renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data) override;

	//pos between [0, 1]
	void addSphericalObstacle(glm::vec3 color, float r) override;

	//pos between [0, 1]
	void addRectengularObstacle(glm::vec3 color, glm::vec3 size) override;

	void removeObstacle() override;

	~SimulationGfx2D() override;

};

} // namespace visual