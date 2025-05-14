#pragma once

#include <glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <unordered_map>
#include <mutex>
#include <set>
#include <optional>
#include <functional>
#include <glm/glm.hpp>

#include "callback.hpp"

namespace renderer
{

/**
 * \brief This class is responsible for a given rendering context, it must be created before any other
 * object in the "renderer" namespace is created and must be destroyed after all other objects are destroyed.
 * \brief You also must make sure that the context is current before creating any object in the "renderer" namespace
 * in the same thread for a given context.
 */
class WindowManager
{
private:
	static std::unordered_map<GLFWwindow*, WindowManager*> managerWindowPairs;
	static std::mutex managerWindowPairsMutex;

	static WindowManager* getManager(GLFWwindow* window);

	static void framebufferSizeChangedCallback(GLFWwindow* window, int width, int height);

	static void mouseCallbackFun(GLFWwindow* window, double xPos, double yPos);
	static void scrollCallbackFun(GLFWwindow* window, double xoffset, double yoffset);
	static void mouseButtonCallbackFun(GLFWwindow* window, int button, int action, int mods);
	static void keyCallbackFun(GLFWwindow* window, int key, int scancode, int action, int mode);

public:
	/**
	* \brief Returns the instance of the WindowManager class that is associated with the current thread context
	* \return The instance of the WindowManager class
	*/
	static WindowManager& getInstance();

	WindowManager(int screenWidth, int screenHeight, std::string name);

	~WindowManager();

	/**
	 * \brief Activates the program with the given id, if the program is already active, nothing happens
	 * \param program - id of the program to activate
	 */
	void activateGPUProgram(unsigned int program);

	/**
	 * \brief Makes the window context current on the calling thread
	 */
	void makeWindowContextcurrent() const;

	/**
	 * \brief Releases the window context on the calling thread
	 */
	void releaseWindowContext() const;

	/**
	 * \brief Swaps the front and back buffers
	 */
	void swapBuffers();

	/**
	 * \brief Returns the length of the last frame in seconds
	 * \return The length of the last frame in seconds
	 */
	double getLastFrameTime() const noexcept;

	unsigned int getScreenWidth() const;
	unsigned int getScreenHeight() const;
	
	/**
	 * \brief Returns the window associated with this manager
	 * 
	 * \return The window associated with this manager
	 */
	GLFWwindow* getWindow() const;

	Callback<double, double> mouseCallback;
	Callback<double, double> scrollCallback;
	Callback<int, int, int> mouseButtonCallback;
	Callback<int, int, int, int> keyCallback;
	Callback<int, int> framebufferSizeCallback;

private:
	GLFWwindow* window;

	unsigned int screenWidth;
	unsigned int screenHeight;

	unsigned int activeProgram = 65535;

	double lastEndTime = 0.0;
	double lastFrameTime = 0.0;
};

} // namespace renderer
