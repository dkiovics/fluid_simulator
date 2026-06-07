#include "controlUi.h"
#include <string_view>
#include "controlCollection.h"
#include "controlRegistryPrivate.h"
#include "engine/glUtils.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "spdlog/spdlog.h"

using namespace controls;

constexpr std::string_view controlValuesFile = "controlValues.registry";

controls::ControlWindow::ControlWindow()
{
    spdlog::info("ControlWindow created");
}

controls::ControlWindow::~ControlWindow()
{
    spdlog::info("ControlWindow destroyed");
}

void controls::ControlWindow::init(renderer::WindowManager& window)
{
    _ControlRegistryPrivate::getInstance().loadControlValues(controlValuesFile.data());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window.getWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 460");

    controlCollection = std::make_unique<_ControlCollection>();

    window.keyCallback.setPrimaryCallback([this](int key, int scancode, int action, int mode)
                                          { return !ImGui::GetIO().WantCaptureKeyboard; });
    window.mouseButtonCallback.setPrimaryCallback([this](int button, int action, int mods)
                                                  { return !ImGui::GetIO().WantCaptureMouse; });
    window.scrollCallback.setPrimaryCallback([this](double xoffset, double yoffset)
                                             { return !ImGui::GetIO().WantCaptureMouse; });
    window.mouseCallback.setPrimaryCallback([this](double xPos, double yPos)
                                            { return !ImGui::GetIO().WantCaptureMouse; });
}

void controls::ControlWindow::render()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    controlCollection->draw();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    _ControlRegistryPrivate::getInstance().syncControlValues();
}

void controls::ControlWindow::shutdown()
{
    controlCollection.reset();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    _ControlRegistryPrivate::getInstance().saveControlValues(controlValuesFile.data());
    spdlog::info("ControlWindow shutdown");
}
