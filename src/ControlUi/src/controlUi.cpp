#include "controlUi.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "spdlog/spdlog.h"
#include "controlCollection.h"
#include "controlRegistryPrivate.h"
#include <string_view>
#include "engine/glUtils.hpp"

using namespace controls;

constexpr std::string_view controlValuesFile = "controlValues.registry";

controls::ControlWindow::ControlWindow(int initialWith, int initialHeight)
	: initialWidth(initialWith), initialHeight(initialHeight)
{
	spdlog::info("ControlWindow created");
}

controls::ControlWindow::~ControlWindow()
{
	stop();
	spdlog::info("ControlWindow destroyed");
}

void controls::ControlWindow::start()
{
	running = true;
	startupFinished = false;
	_ControlRegistryPrivate::getInstance().loadControlValues(controlValuesFile.data());
	renderThread = std::make_unique<std::thread>(&ControlWindow::renderLoop, this);
	
	std::unique_lock lock(mutex);
	startSignal.wait(lock, [this] { return !running || startupFinished; });
}

void controls::ControlWindow::stop() noexcept
{
	running = false;
	if (renderThread && renderThread->joinable())
	{
		renderThread->join();
	}
}

bool controls::ControlWindow::isRunning() const noexcept
{
	return running;
}

void controls::ControlWindow::renderLoop()
{
	//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	std::unique_ptr<renderer::WindowManager> windowManager = std::make_unique<renderer::WindowManager>(initialWidth, initialHeight, "Controls");
	GLFWwindow* window = windowManager->getWindow();
	windowManager->makeWindowContextcurrent();
	glfwSwapInterval(1); // Enable vsync

	std::unique_ptr<_ControlCollection> controlCollection = std::make_unique<_ControlCollection>();

	try
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

		ImGui::StyleColorsDark();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer backends
		bool ok1 = ImGui_ImplGlfw_InitForOpenGL(window, true);
		bool ok2 = ImGui_ImplOpenGL3_Init("#version 460");

		bool show_demo_window = true;
		bool show_another_window = false;
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		while (running && !glfwWindowShouldClose(window))
		{
			// Poll and handle events (inputs, window resize, etc.)
			// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
			// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
			// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
			// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
			glfwPollEvents();
			if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
			{
				ImGui_ImplGlfw_Sleep(10);
				continue;
			}
			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			controlCollection->draw();

			// Rendering
			ImGui::Render();
			int display_w, display_h;
			glfwGetFramebufferSize(window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			renderer::clearViewport(glm::vec4(0.45f, 0.55f, 0.60f, 1.00f));
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// Update and Render additional Platform Windows
			// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
			//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}

			glfwSwapBuffers(window);
			_ControlRegistryPrivate::getInstance().syncControlValues();
			if (!startupFinished)
			{
				std::unique_lock lock(mutex);
				startupFinished = true;
				startSignal.notify_all();
			}
		}

		// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	catch (const std::exception& e)
	{
		spdlog::error("Exception in ControlWindow::renderLoop: {}", e.what());
	}

	windowManager->releaseWindowContext();

	running = false;
	std::unique_lock lock(mutex);
	startSignal.notify_all();
	_ControlRegistryPrivate::getInstance().saveControlValues(controlValuesFile.data());
}
