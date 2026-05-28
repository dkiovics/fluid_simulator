#include "2D/Visuals2D.h"
#include "2D/src/grid.hpp"
#include "controlRegistry.h"
#include "engine/glUtils.hpp"

using namespace visual;

void Visuals2D::mouseCallback(double x, double y)
{
    glm::ivec2 screenSize = subWindowManager->getSize();
    glm::ivec2 screenStart = glm::ivec2(0, 0);

    mousePos =
        glm::vec2((x - screenStart.x) / screenSize.x * 2.0f - 1.0f, (screenStart.y - y) / screenSize.y * 2.0f + 1.0f);
    glm::vec2 d = camera->getWorldPosition(mousePos) - camera->getWorldPosition(prevMousePos);
    prevMousePos = mousePos;
    if (selectedObstacle != -1)
    {
        obstacles[selectedObstacle].movePosition(glm::vec3(d.x, d.y, 0));
    }
}

glm::vec2 Visuals2D::getMouseGridPos() const
{
    return camera->getWorldPosition(mousePos);
}

void Visuals2D::mouseButtonCallback(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        for (int p = 0; p < obstacles.size(); p++)
        {
            glm::vec2 mPos = getMouseGridPos();
            if (obstacles[p].type == Obstacle::ObstacleType::BOX)
            {
                if (fabs(mPos.x - obstacles[p].position.x) < obstacles[p].size.x * 0.5f &&
                    fabs(mPos.y - obstacles[p].position.y) < obstacles[p].size.y * 0.5f)
                {
                    selectedObstacle = p;
                    lastSelectedObstacle = selectedObstacle;
                }
            }
            else if (obstacles[p].type == Obstacle::ObstacleType::SPHERE)
            {
                float distance = glm::length(mPos - glm::vec2(obstacles[p].position));
                if (distance < obstacles[p].size.x * 0.5f)
                {
                    selectedObstacle = p;
                    lastSelectedObstacle = selectedObstacle;
                }
            }
        }
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        selectedObstacle = -1;
    }
}

void Visuals2D::scrollCallback(double xoffset, double yoffset)
{
    float amount = (yoffset > 0) ? 0.5f : -0.5f;
    camera->magnify(mousePos, amount);
}

Visuals2D::Visuals2D(std::shared_ptr<renderer::SubWindowUiManager> subWindowManager, SceneConfig config)
    : GfxInterface(config), subWindowManager(subWindowManager)
{
    camera = std::make_shared<renderer::Camera2D>(glm::vec2(config.simSize), glm::vec2(config.simCenter));

    particleProgram = std::make_shared<renderer::ShaderProgram>("shaders/visualization/2D/particles.vert",
                                                                "shaders/visualization/2D/basic.frag");
    basic2DProgram = std::make_shared<renderer::ShaderProgram>("shaders/visualization/2D/basic.vert",
                                                               "shaders/visualization/2D/basic.frag");

    auto particlesArray = std::make_shared<renderer::InstancedGeometry>(std::make_shared<renderer::Circle>(20));
    particlesGfx = std::make_unique<renderer::Object2D<renderer::InstancedGeometry>>(particlesArray, particleProgram);

    circualrObstacleGfx =
        std::make_unique<renderer::Object2D<renderer::Circle>>(std::make_shared<renderer::Circle>(60), basic2DProgram);
    boxObstacleGfx =
        std::make_unique<renderer::Object2D<renderer::Square>>(std::make_shared<renderer::Square>(), basic2DProgram);

    grid2D = std::make_unique<Grid2D>(config.simSize, config.simCenter, config.gridResolution.x,
                                      config.gridResolution.y, 1.0f);

    camera->addProgram({ particleProgram, basic2DProgram, grid2D->shaderProgram });
    camera->setUniformsForAllPrograms();

    mouseCallbackNum = subWindowManager->mouseCallback.onCallback([this](double x, double y) { mouseCallback(x, y); });
    scrollCallbackNum =
        subWindowManager->scrollCallback.onCallback([this](double x, double y) { scrollCallback(x, y); });
    mouseButtonCallbackNum =
        subWindowManager->mouseButtonCallback.onCallback([this](int a, int b, int c) { mouseButtonCallback(a, b, c); });
}

void Visuals2D::sceneConfigChanged()
{
    camera->init(glm::vec2(sceneConfig.simSize), glm::vec2(sceneConfig.simCenter));
    obstacles.clear();
    selectedObstacle = -1;
}

void Visuals2D::render(glm::ivec2 resolution,
                       renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
                       renderer::fb_ptr fb,
                       pixel_params_ptr _)
{
    if (!fb)
        return;
    fb->bind();

    if (resolution != fb->getSize())
        throw std::runtime_error("Framebuffer size does not match the given resolution.");

    auto& registry = controls::ControlRegistry::getInstance();
    data->bindBuffer(1);

    renderer::setViewport(0, 0, fb->getSize().x, fb->getSize().y);
    renderer::enableDepthTest(false);
    renderer::clearViewport(glm::vec4(0, 0.2, 0, 1));

    if (registry["render.show_grid"])
    {
        grid2D->gridWidth = sceneConfig.gridResolution.x;
        grid2D->gridHeight = sceneConfig.gridResolution.y;
        grid2D->simSize = sceneConfig.simSize;
        grid2D->simCenter = sceneConfig.simCenter;
        grid2D->draw();
    }

    particlesGfx->diffuseColor = glm::vec4((glm::vec3) registry["render.particle_color"], 1.0f);
    float particleRadius = (float) registry["sim.particle_radius"];
    particlesGfx->setScale(glm::vec3(particleRadius, particleRadius, 1));
    particlesGfx->drawable->setInstanceNum(data->getSize());
    (*particleProgram)["coloring.speedColorEnabled"] = (bool) registry["render.particle_speed_color_en"];
    (*particleProgram)["coloring.maxColor"] = glm::vec4((glm::vec3) registry["render.particle_speed_color"], 1.0f);
    (*particleProgram)["coloring.maxSpeed"] = (float) registry["render.max_particle_speed"];
    (*particleProgram)["coloring.exponent"] = (float) registry["render.particle_speed_color_exp"];
    particlesGfx->draw();

    for (auto& o : obstacles)
    {
        if (o.type == Obstacle::ObstacleType::BOX)
        {
            boxObstacleGfx->setPosition(glm::vec4(o.position.x, o.position.y, 0, 1));
            boxObstacleGfx->setScale(glm::vec3(o.size.x, o.size.y, 1));
            boxObstacleGfx->diffuseColor = glm::vec4(o.color, 1);
            boxObstacleGfx->draw();
        }
        else if (o.type == Obstacle::ObstacleType::SPHERE)
        {
            circualrObstacleGfx->setPosition(glm::vec4(o.position.x, o.position.y, 0, 1));
            circualrObstacleGfx->setScale(glm::vec3(o.size.x * 0.5f, o.size.x * 0.5f, 1));
            circualrObstacleGfx->diffuseColor = glm::vec4(o.color, 1);
            circualrObstacleGfx->draw();
        }
    }
}

void Visuals2D::handleUserInteractions()
{
    auto& registry = controls::ControlRegistry::getInstance();

    if (registry["obs.add_sphere"])
    {
        obstacles.push_back(Obstacle((float) registry["obs.obstacle_radius"], sceneConfig.simCenter,
                                     (glm::vec3) registry["obs.color"]));
        registry["obs.add_sphere"] = false;
    }
    if (registry["obs.add_rectangle"])
    {
        obstacles.push_back(
            Obstacle(glm::vec3((float) registry["obs.obstacle_x"], (float) registry["obs.obstacle_y"], 0),
                     sceneConfig.simCenter, (glm::vec3) registry["obs.color"]));
        registry["obs.add_rectangle"] = false;
    }
    if (registry["obs.remove"])
    {
        if (lastSelectedObstacle != -1)
        {
            obstacles.erase(obstacles.begin() + lastSelectedObstacle);
            lastSelectedObstacle = -1;
        }
        else if (obstacles.size() > 0)
        {
            obstacles.pop_back();
        }
        registry["obs.remove"] = false;
    }

    for (auto& o : obstacles)
        o.handleNoPositionChange();
}

Visuals2D::~Visuals2D()
{
    spdlog::debug("Visuals2D deleted");
    subWindowManager->mouseCallback.removeCallbackFunction(mouseCallbackNum);
    subWindowManager->mouseButtonCallback.removeCallbackFunction(mouseButtonCallbackNum);
    subWindowManager->scrollCallback.removeCallbackFunction(scrollCallbackNum);
}
