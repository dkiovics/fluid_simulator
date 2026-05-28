#pragma once

#include <engine/windowManager.h>
#include <engineUtils/object.h>
#include <geometries/basicGeometries.h>
#include <manager/simulationManager.h>
#include <algorithm>
#include <engineUtils/camera2D.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gfxInterface.hpp"

namespace visual
{

class Grid2D;

class Visuals2D : public GfxInterface
{
private:
    std::shared_ptr<renderer::SubWindowUiManager> subWindowManager;

    /*glm::mat4 P;
    float magnification = 1;
    glm::vec3 magPos = glm::vec3(0.5, 0.5, 0);*/
    // In normalized device coordinates
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
    Visuals2D(std::shared_ptr<renderer::SubWindowUiManager> subWindowManager, SceneConfig initialConfig);

    void render(glm::ivec2 resolution,
        renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
                renderer::fb_ptr fb = nullptr,
                pixel_params_ptr pixelParamBuffers = nullptr) override;

    void handleUserInteractions() override;

    ~Visuals2D() override;

protected:
    void sceneConfigChanged() override;
};

} // namespace visual
