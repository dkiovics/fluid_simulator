#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <engine/renderEngine.h>
#include "ui/simulationGui.h"


int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	startSimulatorGui();

	glfwTerminate();
	return 0;
}


