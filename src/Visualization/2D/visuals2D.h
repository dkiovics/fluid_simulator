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

class Grid2D;

class Visuals2D : public GfxInterface
{
private:
	renderer::WindowManager& window;

	/*glm::mat4 P;
	float magnification = 1;
	glm::vec3 magPos = glm::vec3(0.5, 0.5, 0);*/
	//In normalized device coordinates
	glm::vec2 mousePos = glm::vec2(0);
	glm::vec2 prevMousePos = glm::vec2(0);

	std::shared_ptr<renderer::Camera2D> camera;

	std::shared_ptr<renderer::GpuProgram> particleProgram;
	std::shared_ptr<renderer::GpuProgram> basic2DProgram;

	std::unique_ptr<renderer::Object2D<renderer::InstancedGeometry>> particlesGfx;

	std::unique_ptr<renderer::Object2D<renderer::Circle>> circualrObstacleGfx;
	std::unique_ptr<renderer::Object2D<renderer::Square>> boxObstacleGfx;

	std::unique_ptr<Grid2D> grid2D;

	int selectedObstacle = -1;
	int lastSelectedObstacle = -1;

	bool topIsSolid = false;

	int mouseCallbackNum;
	int mouseButtonCallbackNum;
	int scrollCallbackNum;

private:
	void mouseCallback(double x, double y);

	void mouseButtonCallback(int button, int action, int mods);

	void scrollCallback(double xoffset, double yoffset);

	void handleObstacles();

	glm::vec2 getMouseGridPos() const;

	void addSphericalObstacle(glm::vec3 color, float r);

public:
	Visuals2D(renderer::WindowManager& window, SceneConfig initialConfig);

	void render(renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data, 
		renderer::fb_ptr fb = nullptr, renderer::ssbo_ptr<PixelParamData> pixelParamData = nullptr) override;

	void handleUserInteractions() override;

	~Visuals2D() override;

protected:
	void sceneConfigChanged() override;

};

} // namespace visual