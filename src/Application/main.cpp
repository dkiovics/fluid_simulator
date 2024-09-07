#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <engine/renderEngine.h>
#include "ui/simulationGui.h"

#include <spdlog/spdlog.h>


int main() {
#ifdef _DEBUG
	spdlog::set_level(spdlog::level::debug);
	spdlog::debug("Debug mode");
#endif

	
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	spdlog::info("Starting application...");

#ifndef _DEBUG
	try
	{
#endif
		startSimulatorGui();
#ifndef _DEBUG
	}
	catch (const std::exception& e)
	{
		spdlog::error("An error occurred: {}", e.what());
	}
	catch (...)
	{
		spdlog::error("An unknown error occurred.");
	}
#endif

	spdlog::info("Terminating application...");
	glfwTerminate();
	spdlog::info("Application terminated.");
	return 0;
}


