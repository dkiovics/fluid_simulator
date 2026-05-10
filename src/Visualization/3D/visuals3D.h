#pragma once

#include <engine/framebuffer.h>
#include <engine/shaderProgram.h>
#include <engine/texture.h>
#include <engine/windowManager.h>
#include <engineUtils/object.h>
#include <geometries/basicGeometries.h>
#include <engineUtils/camera3D.hpp>
#include <engineUtils/lights.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "gfxInterface.hpp"

namespace visual
{

class TransparentBox;

class FluidRenderer;

class Visuals3D : public GfxInterface
{
private:
    const std::shared_ptr<renderer::SubWindowUiManager> subWindowManager;

    std::shared_ptr<renderer::Camera3D> camera;
    std::shared_ptr<renderer::Lights> lights;

    int selectedObstacle = -1;
    int lastSelectedObstacle = -1;

    std::unique_ptr<renderer::Object3D<renderer::Geometry>> planeGfx;
    std::shared_ptr<renderer::Square> fullScreenQuad;
    std::unique_ptr<TransparentBox> transparentBox;

    std::unique_ptr<renderer::Object3D<renderer::Sphere>> sphereGfx;
    std::unique_ptr<renderer::Object3D<renderer::Cube>> boxGfx;
    std::vector<std::unique_ptr<renderer::Object3D<renderer::MeshGeometry>>> meshes;

    std::shared_ptr<FluidRenderer> fluidRenderer;

    std::shared_ptr<renderer::GpuProgram> shaderProgramNotTextured;
    std::shared_ptr<renderer::GpuProgram> shaderProgramTextured;
    std::shared_ptr<renderer::GpuProgram> fullScreenShader;
    std::shared_ptr<renderer::Framebuffer> renderTargetFramebuffer;

    int mouseCallbackId;
    int mouseButtonCallbackId;
    int scrollCallbackId;
    int keyCallbackId;
    int framebufferSizeCallbackId;
    bool keys[1024] = { 0 };

    glm::vec2 mousePos = glm::vec2(0.5, 0.5);
    glm::vec2 prevMousePos = glm::vec2(0.5, 0.5);
    bool isLeftButtonDown = false;
    bool isMiddleButtonDown = false;
    bool isRightButtonDown = false;

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
    void screenResolutionChanged(int width, int height);

protected:
    void sceneConfigChanged() override;

public:
    Visuals3D(std::shared_ptr<renderer::SubWindowUiManager> subWindowManager, SceneConfig config);

    void handleUserInteractions() override;

    void render(glm::ivec2 resolution,
                renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
                renderer::fb_ptr canvas = nullptr,
                pixel_params_ptr pixelParamBuffers = nullptr) override;

    bool renderBoxFrontEnabled = true;

    std::shared_ptr<renderer::Camera3D> getCamera() const { return camera; }

    ~Visuals3D() override;
};

} // namespace visual
