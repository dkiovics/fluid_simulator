#include "3D/visuals3D.h"
#include <geometries/basicGeometries.h>
#include <algorithm>
#include <map>
#include "controlRegistry.h"
#include "engine/glUtils.hpp"
#include "fluidParticles.h"
#include "fluidSurface.h"
#include "imgui.h"
#include "transparentBox.hpp"

using namespace visual;

void Visuals3D::mouseCallback(double x, double y)
{
    glm::ivec2 screenStart(0, 0);
    glm::ivec2 screenSize(subWindowManager->getSize());

    mousePos = glm::vec2((x - screenStart.x) / screenSize.x, (screenStart.y - y) / screenSize.y + 1.0f);
    if (mousePos.x < 0 || mousePos.x > 1 || mousePos.y < 0 || mousePos.y > 1)
    {
        isLeftButtonDown = false;
        isRightButtonDown = false;
        isMiddleButtonDown = false;
        selectedObstacle = -1;
        return;
    }
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

void Visuals3D::mouseButtonCallback(int button, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureMouse)
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

void Visuals3D::scrollCallback(double xoffset, double yoffset)
{
    if (ImGui::GetIO().WantCaptureMouse)
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

void Visuals3D::keyCallback(int key, int scancode, int action, int mode)
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

Visuals3D::Visuals3D(std::shared_ptr<renderer::SubWindowUiManager> subWindowManager, SceneConfig config)
    : GfxInterface(config), subWindowManager(subWindowManager)
{
    auto& registry = controls::ControlRegistry::getInstance();

    glm::ivec2 screenSize(subWindowManager->getSize());
    modelRotationPoint = glm::vec3(config.simCenter);

    shaderProgramNotTextured = std::make_shared<renderer::ShaderProgram>(
        "shaders/visualization/3D/object.vert", "shaders/visualization/3D/object_not_textured.frag");
    shaderProgramTextured = std::make_shared<renderer::ShaderProgram>("shaders/visualization/3D/object.vert",
                                                                      "shaders/visualization/3D/object_textured.frag");
    fullScreenShader =
        std::make_shared<renderer::ShaderProgram>("shaders/utils/fullScreen.vert", "shaders/utils/fullScreen.frag");

    fullScreenQuad = std::make_shared<renderer::Square>();
    renderer::render_target_ptr fullScreenTexture = renderer::make_render_target(
        screenSize.x, screenSize.y, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    (*fullScreenShader)["colorTexture"] = *fullScreenTexture;
    std::vector<std::shared_ptr<renderer::RenderTargetTexture>> textures = { fullScreenTexture };
    std::shared_ptr<renderer::RenderTargetTexture> renderTargetDepthTexture =
        std::make_shared<renderer::RenderTargetTexture>(screenSize.x, screenSize.y, GL_NEAREST, GL_NEAREST,
                                                        GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);
    renderTargetFramebuffer = std::make_shared<renderer::Framebuffer>(textures, renderTargetDepthTexture, false);

    keyCallbackId =
        subWindowManager->keyCallback.onCallback([this](int a, int b, int c, int d) { keyCallback(a, b, c, d); });
    mouseCallbackId = subWindowManager->mouseCallback.onCallback([this](double a, double b) { mouseCallback(a, b); });
    mouseButtonCallbackId =
        subWindowManager->mouseButtonCallback.onCallback([this](int a, int b, int c) { mouseButtonCallback(a, b, c); });
    scrollCallbackId =
        subWindowManager->scrollCallback.onCallback([this](double a, double b) { scrollCallback(a, b); });

    camera =
        std::make_unique<renderer::Camera3D>(glm::vec3(10.0f, 10.0f, 0.0f), 40.0f, float(screenSize.x) / screenSize.y);
    camera->rotateAroundPoint(modelRotationPoint, cameraDistance, -20, 40);

    lights = std::make_shared<renderer::Lights>();
    lights->lights.push_back(std::make_unique<renderer::Light>(glm::vec4(50, 50, 50, 1), glm::vec3(1000, 2000, 2000)));
    lights->lights.push_back(std::make_unique<renderer::Light>(glm::vec4(50, 50, -50, 1), glm::vec3(2000, 1000, 2000)));
    lights->lights.push_back(
        std::make_unique<renderer::Light>(glm::vec4(-50, 50, -50, 1), glm::vec3(2000, 2000, 1000)));
    lights->lights.push_back(std::make_unique<renderer::Light>(glm::vec4(-50, 50, 50, 1), glm::vec3(2000, 2000, 2000)));
    lights->setUniformsForAllPrograms();

    if ((int) registry["render.fluid_render_mode"] == 1)
    {
        fluidRenderer = std::make_shared<FluidParticles>(screenSize, camera, lights);
    }
    else if ((int) registry["render.fluid_render_mode"] == 2)
    {
        fluidRenderer = std::make_shared<FluidSurface>(screenSize, camera, lights);
    }

    auto floorTexture =
        std::make_shared<renderer::ColorTexture>("textures/tiles.jpg", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, true);
    auto floor = std::make_shared<renderer::Square>();
    planeGfx = std::make_unique<renderer::Object3D<renderer::Geometry>>(floor, shaderProgramTextured);
    planeGfx->colorTextureScale = 5.0;
    planeGfx->colorTexture = floorTexture;
    planeGfx->shininess = 5.8f;
    planeGfx->specularColor = glm::vec4(0.7, 0.7, 0.7, 1);
    planeGfx->diffuseColor = glm::vec4(1, 1, 1, 1);
    planeGfx->setScale(glm::vec3(200, 200, 1));
    planeGfx->setPitch(PI * 0.5f);

    boxGfx = std::make_unique<renderer::Object3D<renderer::Cube>>(std::make_shared<renderer::Cube>(),
                                                                  shaderProgramNotTextured);
    sphereGfx = std::make_unique<renderer::Object3D<renderer::Sphere>>(std::make_shared<renderer::Sphere>(80),
                                                                       shaderProgramNotTextured);
    boxGfx->shininess = 80;
    boxGfx->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);
    sphereGfx->shininess = 80;
    sphereGfx->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);

    transparentBox = std::make_unique<TransparentBox>(camera, glm::vec4(0.5, 0.5, 0.65, 0.3), shaderProgramNotTextured);

    camera->addProgram({ shaderProgramNotTextured, shaderProgramTextured });
    lights->addProgram({ shaderProgramNotTextured, shaderProgramTextured });
    camera->setUniformsForAllPrograms();
    lights->setUniformsForAllPrograms();
}

// void Visuals3D::addObstacle(std::unique_ptr<renderer::Object3D<renderer::Geometry>> obstacleGfx,
// std::unique_ptr<genericfsim::manager::Obstacle> obstacle, glm::dvec3 size)
//{
//	glm::dvec3 center = simulationManager->getDimensions() * 0.5;
//
//	obstacleGfx->setPosition(glm::vec4(center, 1));
//	obstacleGfx->shininess = 80;
//	obstacleGfx->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);
//	this->obstacleGfxArray.push_back(std::move(obstacleGfx));
//	obstacleHitboxes.push_back({ center, size });
//
//	auto obstacles = simulationManager->getObstacles();
//	obstacle->pos = center;
//	obstacles.push_back(std::move(obstacle));
//	simulationManager->setObstacles(std::move(obstacles));
// }
//
// void Visuals3D::addSphericalObstacle(glm::vec3 color, float r)
//{
//	auto obj = std::make_unique<renderer::Object3D<renderer::Geometry>>(std::make_shared<renderer::Sphere>(80),
// shaderProgramNotTextured); 	obj->setScale(glm::vec3(r, r, r)); 	obj->diffuseColor = glm::vec4(color, 1);
//	addObstacle(std::move(obj), std::make_unique<genericfsim::obstacle::SphericalObstacle>(r), glm::vec3(r * 2, r * 2, r
//* 2));
// }
//
// void Visuals3D::addParticleSource(glm::vec3 color, float r, float particleSpawnRate, float particleSpawnSpeed)
//{
//	auto obj = std::make_unique<renderer::Object3D<renderer::Geometry>>(std::make_shared<renderer::Sphere>(80),
// shaderProgramNotTextured); 	obj->setScale(glm::vec3(r, r, r)); 	obj->diffuseColor = glm::vec4(color, 1);
//	addObstacle(std::move(obj), std::make_unique<genericfsim::obstacle::SphericalParticleSource>(r, particleSpawnRate,
// particleSpawnSpeed), glm::vec3(r * 2, r * 2, r * 2));
// }
//
// void Visuals3D::addParticleSink(glm::vec3 color, float r)
//{
//	auto obj = std::make_unique<renderer::Object3D<renderer::Geometry>>(std::make_shared<renderer::Sphere>(80),
// shaderProgramNotTextured); 	obj->setScale(glm::vec3(r, r, r)); 	obj->diffuseColor = glm::vec4(color, 1);
//	addObstacle(std::move(obj), std::make_unique<genericfsim::obstacle::SphericalParticleSink>(r), glm::vec3(r * 2, r *
// 2, r * 2));
// }
//
// void Visuals3D::addRectengularObstacle(glm::vec3 color, glm::vec3 size)
//{
//	auto obj = std::make_unique<renderer::Object3D<renderer::Geometry>>(std::make_shared<renderer::Cube>(),
// shaderProgramNotTextured); 	obj->setScale(size); 	obj->diffuseColor = glm::vec4(color, 1);
// addObstacle(std::move(obj), std::make_unique<genericfsim::obstacle::RectengularObstacle>(size), size);
// }
//
// void Visuals3D::addMeshObstacle(const std::string& path, glm::vec3 color, float scale)
//{
//	try
//	{
//		auto obj = std::make_unique<renderer::Object3D<renderer::Geometry>>
//			(renderer::loadGeometry(path), shaderProgramNotTextured);
//		obj->setScale(glm::vec3(scale, scale, scale));
//		obj->diffuseColor = glm::vec4(color, 1);
//		addObstacle(std::move(obj), std::make_unique<genericfsim::obstacle::SphericalObstacle>(4.0f),
// glm::vec3(4.0f, 4.0f, 4.0f));
//	}
//	catch (std::exception& e)
//	{
//		spdlog::error("Visuals3D::addMeshObstacle: {}", e.what());
//	}
// }
//
// void Visuals3D::removeObstacle()
//{
//	if (lastSelectedObstacle == -1)
//		lastSelectedObstacle = (int)obstacleGfxArray.size() - 1;
//	if (lastSelectedObstacle == -1)
//		return;
//	auto obstacles = simulationManager->getObstacles();
//	obstacles.erase(obstacles.begin() + lastSelectedObstacle);
//	simulationManager->setObstacles(std::move(obstacles));
//	obstacleGfxArray.erase(obstacleGfxArray.begin() + lastSelectedObstacle);
//	obstacleHitboxes.erase(obstacleHitboxes.begin() + lastSelectedObstacle);
//	lastSelectedObstacle = -1;
// }

static glm::vec3 findIntersection(const glm::vec3& planePoint,
                                  const glm::vec3& planeNormal,
                                  const glm::vec3& linePoint,
                                  const glm::vec3& lineDirection)
{
    float dotNumerator = glm::dot(planeNormal, (planePoint - linePoint));
    float dotDenominator = glm::dot(planeNormal, lineDirection);
    if (glm::abs(dotDenominator) < 1e-6f)
    {
        return glm::vec3(0, 0, 0);
    }
    float t = dotNumerator / dotDenominator;
    return linePoint + t * lineDirection;
}

static float findIntersection(const glm::vec3& sphereCenter,
                              const float sphereRadius,
                              const glm::vec3& linePoint,
                              const glm::vec3& lineDirection)
{
    glm::vec3 oc = linePoint - sphereCenter;
    float a = glm::dot(lineDirection, lineDirection);
    float b = 2.0f * glm::dot(oc, lineDirection);
    float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0)
    {
        return -1.0f;
    }
    else
    {
        float t = (-b - sqrt(discriminant)) / (2.0f * a);
        if (t <= 0)
            return -1.0f;
        return t;
    }
}

void Visuals3D::selectObstacle()
{
    const glm::vec3 rayDir = camera->getMouseRayDir(mousePos);
    const glm::vec3 camPos = camera->getPosition();

    float closestPoint = 1e8;
    selectedObstacle = -1;
    for (int i = 0; i < obstacles.size(); i++)
    {
        auto& o = obstacles[i];
        if (o.type == Obstacle::ObstacleType::SPHERE)
        {
            float t = findIntersection(o.position, o.size.x * 0.5f, camPos, rayDir);
            if (t > 0)
            {
                glm::vec3 intersection = camPos + t * rayDir;
                float dist = glm::length(intersection - camPos);
                if (dist < closestPoint)
                {
                    closestPoint = dist;
                    selectedObstacle = i;
                    lastSelectedObstacle = selectedObstacle;
                }
            }
        }
        else if (o.type == Obstacle::ObstacleType::BOX)
        {
            const glm::vec3 center = obstacles[i].position;
            const glm::vec3 size = obstacles[i].size;
            for (int axis = 0; axis < 3; axis++)
            {
                for (int sign = -1; sign <= 1; sign += 2)
                {
                    glm::vec3 planePoint = center;
                    planePoint[axis] += sign * size[axis] * 0.5f;
                    glm::vec3 planeNormal(0, 0, 0);
                    planeNormal[axis] = (float) sign;
                    glm::vec3 intersection = findIntersection(planePoint, planeNormal, camPos, rayDir);
                    glm::ivec3 ignoreAxis(0, 0, 0);
                    ignoreAxis[axis] = 1;
                    if ((intersection.x >= center.x - size.x * 0.5f && intersection.x <= center.x + size.x * 0.5 ||
                         ignoreAxis.x) &&
                        (intersection.y >= center.y - size.y * 0.5f && intersection.y <= center.y + size.y * 0.5 ||
                         ignoreAxis.y) &&
                        (intersection.z >= center.z - size.z * 0.5f && intersection.z <= center.z + size.z * 0.5 ||
                         ignoreAxis.z))
                    {
                        float dist = glm::length(intersection - camPos);
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
}

void Visuals3D::handleObstacleMovement()
{
    if (selectedObstacle == -1)
        return;
    glm::vec3 obstacleCenter = obstacles[selectedObstacle].position;
    glm::vec3 camDir = glm::normalize(camera->getFrontVec());
    glm::vec3 camPos = camera->getPosition();
    glm::vec3 prevRayDir = camera->getMouseRayDir(prevMousePos);
    glm::vec3 rayDir = camera->getMouseRayDir(mousePos);
    glm::vec3 prevIntersection = findIntersection(obstacleCenter, camDir, camPos, prevRayDir);
    glm::vec3 intersection = findIntersection(obstacleCenter, camDir, camPos, rayDir);
    glm::vec3 movement = intersection - prevIntersection;
    obstacles[selectedObstacle].movePosition(movement);
}

void Visuals3D::sceneConfigChanged()
{
    modelRotationPoint = glm::vec3(sceneConfig.simCenter);
}

void Visuals3D::screenResolutionChanged(int width, int height)
{
    renderTargetFramebuffer->setSize(glm::ivec2(width, height));
    camera->setAspectRatio(float(width) / height);
    if (fluidRenderer)
    {
        fluidRenderer->setScreenResolution(glm::ivec2(width, height));
    }
}

void Visuals3D::render(glm::ivec2 resolution,
                       renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
                       renderer::fb_ptr fb,
                       pixel_params_ptr pixelParamBuffers)
{
    if (!fb && !pixelParamBuffers)
        throw std::runtime_error("No framebuffer to render to");

    if (fb && resolution != fb->getSize())
        throw std::runtime_error("Framebuffer size does not match screen resolution");

    auto& registry = controls::ControlRegistry::getInstance();
    if (resolution != renderTargetFramebuffer->getSize())
    {
        screenResolutionChanged(resolution.x, resolution.y);
    }

    if (pixelParamBuffers != nullptr && pixelParamBuffers->getScreenSize() != resolution)
        throw std::runtime_error("PixelParamBuffers size does not match screen size");

    if ((int) registry["render.fluid_render_mode"] == 1 && !dynamic_cast<FluidParticles*>(fluidRenderer.get()))
    {
        fluidRenderer = std::make_shared<FluidParticles>(resolution, camera, lights);
    }
    else if ((int) registry["render.fluid_render_mode"] == 2 && !dynamic_cast<FluidSurface*>(fluidRenderer.get()))
    {
        fluidRenderer = std::make_shared<FluidSurface>(resolution, camera, lights);
    }

    renderTargetFramebuffer->bind();

    renderer::enableDepthTest(true);
    renderer::setViewport(0, 0, resolution.x, resolution.y);
    renderer::clearViewport(glm::vec4(0, 0, 0, 0), 1.0f);

    planeGfx->setPosition(glm::vec4(sceneConfig.simCenter.x, -0.05f, sceneConfig.simCenter.z, 1));
    planeGfx->draw();

    if (registry["obs.show_obstacles"])
    {
        for (auto& o : obstacles)
        {
            // Mesh obstacles are constructed with the float (SPHERE) Obstacle ctor, so
            // meshIndex must be checked before the obstacle type — otherwise a mesh
            // obstacle would fall into the SPHERE branch and render as a sphere.
            if (o.meshIndex != -1)
            {
                meshes[o.meshIndex]->setPosition(glm::vec4(o.position, 1));
                meshes[o.meshIndex]->draw();
            }
            else if (o.type == Obstacle::ObstacleType::BOX)
            {
                boxGfx->setPosition(glm::vec4(o.position, 1));
                boxGfx->setScale(o.size);
                boxGfx->diffuseColor = glm::vec4(o.color, 1);
                boxGfx->draw();
            }
            else if (o.type == Obstacle::ObstacleType::SPHERE)
            {
                sphereGfx->setPosition(glm::vec4(o.position, 1));
                sphereGfx->setScale(glm::vec3(o.size.x * 0.5f, o.size.x * 0.5f, o.size.x * 0.5f));
                sphereGfx->diffuseColor = glm::vec4(o.color, 1);
                sphereGfx->draw();
            }
        }
    }

    if (registry["render.show_box"])
    {
        transparentBox->draw(sceneConfig.simCenter, sceneConfig.simSize, true, false);
    }

    fluidRenderer->renderFluid(subWindowManager->getLastFrameTime(), data, renderTargetFramebuffer, pixelParamBuffers);

    renderTargetFramebuffer->bind();

    if (registry["render.show_box"] && renderBoxFrontEnabled)
    {
        transparentBox->draw(sceneConfig.simCenter, sceneConfig.simSize, false, true);
    }

    if (fb)
    {
        fb->bind();
        renderer::setViewport(0, 0, resolution.x, resolution.y);

        fullScreenShader->activate();
        renderer::enableDepthTest(false);
        fullScreenQuad->draw();
        renderer::enableDepthTest(true);
    }
}

void Visuals3D::handleUserInteractions()
{
    auto& registry = controls::ControlRegistry::getInstance();
    float dt = (float) subWindowManager->getLastFrameTime();

    glm::vec3 forward = camera->getFrontVec();
    forward = glm::normalize(glm::vec3(forward.x, 0, forward.z));
    glm::vec3 right = camera->getRightVec();
    const float movementSpeed = (float) dt * 20.0f;
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

    if (registry["obs.add_sphere"])
    {
        obstacles.push_back(Obstacle((float) registry["obs.obstacle_radius"], sceneConfig.simCenter,
                                     (glm::vec3) registry["obs.color"]));
        registry["obs.add_sphere"] = false;
    }
    if (registry["obs.add_rectangle"])
    {
        obstacles.push_back(Obstacle(glm::vec3((float) registry["obs.obstacle_x"], (float) registry["obs.obstacle_y"],
                                               (float) registry["obs.obstacle_z"]),
                                     sceneConfig.simCenter, (glm::vec3) registry["obs.color"]));
        registry["obs.add_rectangle"] = false;
    }
    if (registry["obs.add_mesh"])
    {
        try
        {
            float scale = (float) registry["obs.mesh_scale"];
            auto obj = std::make_unique<renderer::Object3D<renderer::MeshGeometry>>(
                renderer::loadGeometry(registry["obs.mesh_file"].operator std::string()), shaderProgramNotTextured);
            obj->setScale(glm::vec3(scale));
            obj->diffuseColor = glm::vec4((glm::vec3) registry["obs.color"], 1);
            // Match boxGfx / sphereGfx — without these, the mesh leaves the shader's
            // shininess/specularColor uniforms at whatever was last written (e.g. the
            // TransparentBox's shininess=10), and that state persists across frames.
            obj->shininess = 80.0f;
            obj->specularColor = glm::vec4(1.2f, 1.2f, 1.2f, 1.0f);

            obstacles.push_back(Obstacle(scale < 5.0f ? 5.0f : scale, sceneConfig.simCenter, (glm::vec3) registry["obs.color"]));
            obstacles[obstacles.size() - 1].meshIndex = (int) meshes.size();
            meshes.push_back(std::move(obj));
        }
        catch (std::exception& e)
        {
            spdlog::error("Visuals3D::addMeshObstacle: {}", e.what());
        }
        registry["obs.add_mesh"] = false;
    }
    if (registry["obs.remove"])
    {
        int meshIndexToRemove = -1;
        if (lastSelectedObstacle != -1)
        {
            if (obstacles[lastSelectedObstacle].meshIndex != -1)
            {
                meshIndexToRemove = obstacles[lastSelectedObstacle].meshIndex;
            }
            obstacles.erase(obstacles.begin() + lastSelectedObstacle);
            lastSelectedObstacle = -1;
        }
        else if (obstacles.size() > 0)
        {
            if (obstacles.back().meshIndex != -1)
            {
                meshIndexToRemove = obstacles.back().meshIndex;
            }
            obstacles.pop_back();
        }
        registry["obs.remove"] = false;
        meshes.erase(meshes.begin() + meshIndexToRemove);
    }

    for (auto& o : obstacles)
    {
        o.handleNoPositionChange();
    }
}

Visuals3D::~Visuals3D()
{
    spdlog::debug("Visuals3D destructor");
    subWindowManager->mouseCallback.removeCallbackFunction(mouseCallbackId);
    subWindowManager->mouseButtonCallback.removeCallbackFunction(mouseButtonCallbackId);
    subWindowManager->scrollCallback.removeCallbackFunction(scrollCallbackId);
    subWindowManager->keyCallback.removeCallbackFunction(keyCallbackId);
}
