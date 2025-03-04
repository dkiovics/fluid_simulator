#include "renderEngine.h"

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>


using namespace renderer;

std::unordered_map<GLFWwindow*, WindowManager*> WindowManager::managerWindowPairs;

void WindowManager::framebufferSizeChangedCallback(GLFWwindow* window, int width, int height)
{
	auto tmp = getManager(window);
	if (!tmp)
		return;
	WindowManager& manager = *tmp;

	manager.screenWidth = width;
	manager.screenHeight = height;
	manager.framebufferSizeCallback.triggerCallback(width, height);
}

void WindowManager::mouseCallbackFun(GLFWwindow* window, double x, double y)
{
	auto tmp = getManager(window);
	if (tmp)
		tmp->mouseCallback.triggerCallback(x, y);
}

void WindowManager::scrollCallbackFun(GLFWwindow* window, double xOffset, double yOffset)
{
	auto tmp = getManager(window);
	if (tmp)
		tmp->scrollCallback.triggerCallback(xOffset, yOffset);
}

void WindowManager::mouseButtonCallbackFun(GLFWwindow* window, int button, int action, int mods)
{
	auto tmp = getManager(window);
	if (tmp)
		tmp->mouseButtonCallback.triggerCallback(button, action, mods);
}

void WindowManager::keyCallbackFun(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	auto tmp = getManager(window);
	if (tmp)
		tmp->keyCallback.triggerCallback(key, scancode, action, mode);
}

WindowManager* WindowManager::getManager(GLFWwindow* window)
{
	auto pair = managerWindowPairs.find(window);
	if (pair == managerWindowPairs.end())
		return nullptr;
	return pair->second;
}

WindowManager& renderer::WindowManager::getInstance()
{
	GLFWwindow* window = glfwGetCurrentContext();
	return *WindowManager::getManager(window);
}

WindowManager::WindowManager(int screenWidth, int screenHeight, std::string name) : screenWidth(screenWidth), screenHeight(screenHeight)
{
	window = glfwCreateWindow(screenWidth, screenHeight, name.c_str(), NULL, NULL);
	if (window == NULL)
		throw std::runtime_error("Failed to create window");
	managerWindowPairs.insert(std::make_pair(window, this));
	glfwMakeContextCurrent(window);
	glfwSetCursorPosCallback(window, mouseCallbackFun);
	glfwSetScrollCallback(window, scrollCallbackFun);
	glfwSetMouseButtonCallback(window, mouseButtonCallbackFun);
	glfwSetKeyCallback(window, keyCallbackFun);
	glfwSetFramebufferSizeCallback(window, framebufferSizeChangedCallback);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw std::runtime_error("Failed to init GLAD");
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	int maxImageUnits;
	glGetIntegerv(GL_MAX_IMAGE_UNITS, &maxImageUnits);
	spdlog::info("Max image units: {}", maxImageUnits);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxImageUnits);
	spdlog::info("Max combined texture image units: {}", maxImageUnits);
}

WindowManager::~WindowManager()
{
	glfwMakeContextCurrent(window);
	glfwDestroyWindow(window);
	spdlog::info("WindowManager destroyed");
}

void WindowManager::activateGPUProgram(unsigned int program)
{
	if (activeProgram == program)
		return;
	glUseProgram(program);
	activeProgram = program;
}

void WindowManager::renderWireframeOnly(bool enable)
{
	if (enable)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void renderer::WindowManager::enableDepthTest(bool enable)
{
	if (enable)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

void renderer::WindowManager::clearViewport(const glm::vec4& color, const float depth)
{
	glClearColor(color.x, color.y, color.z, color.w);
	glClearDepth(depth);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer::WindowManager::clearViewport(const glm::vec4& color)
{
	glClearColor(color.x, color.y, color.z, color.w);
	glClear(GL_COLOR_BUFFER_BIT);
}

void renderer::WindowManager::clearViewport(const float depth)
{
	glClearDepth(depth);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void WindowManager::makeWindowContextcurrent()
{
	glfwMakeContextCurrent(window);
}

void renderer::WindowManager::swapBuffers()
{
	glfwSwapBuffers(window);
	double currentTime = glfwGetTime();
	lastFrameTime = currentTime - lastEndTime;
	lastEndTime = currentTime;
}

double renderer::WindowManager::getLastFrameTime() const
{
	return lastFrameTime;
}

void WindowManager::setViewport(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}

void renderer::WindowManager::bindDefaultFramebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int WindowManager::getScreenWidth() const
{
	return screenWidth;
}

unsigned int WindowManager::getScreenHeight() const
{
	return screenHeight;
}

GLFWwindow* WindowManager::getWindow() const
{
	return window;
}
