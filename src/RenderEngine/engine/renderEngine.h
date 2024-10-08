#pragma once

#include <glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <unordered_map>
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
class RenderEngine
{
private:
	static std::unordered_map<GLFWwindow*, RenderEngine*> engineWindowPairs;

	static RenderEngine* getEngine(GLFWwindow* window);

	static void framebufferSizeChangedCallback(GLFWwindow* window, int width, int height);

	static void mouseCallbackFun(GLFWwindow* window, double xPos, double yPos);
	static void scrollCallbackFun(GLFWwindow* window, double xoffset, double yoffset);
	static void mouseButtonCallbackFun(GLFWwindow* window, int button, int action, int mods);
	static void keyCallbackFun(GLFWwindow* window, int key, int scancode, int action, int mode);

public:
	/**
	* \brief Returns the instance of the RenderEngine class that is associated with the current thread context
	* \return The instance of the RenderEngine class
	*/
	static RenderEngine& getInstance();

	RenderEngine(int screenWidth, int screenHeight, std::string name);

	~RenderEngine();

	/**
	 * \brief Activates the program with the given id, if the program is already active, nothing happens
	 * \param program - id of the program to activate
	 */
	void activateGPUProgram(unsigned int program);

	/**
	 * \brief Renders wireframe only if enable is true
	 * \param enable - if true, wireframe is rendered, if false, normal rendering is done
	 */
	void renderWireframeOnly(bool enable);

	/**
	 * \brief Enables or disables depth test
	 * \param enable - if true, depth test is enabled, if false, depth test is disabled
	 */
	void enableDepthTest(bool enable);

	/**
	 * \brief Clear the viewport color and depth buffer with the given values
	 * \param color - color to clear the viewport with
	 * \param depth - depth value to clear the viewport with
	 */
	void clearViewport(const glm::vec4& color, const float depth);
	
	/**
	 * \brief Clear the viewport color buffer with the given color
	 * \param color - color to clear the viewport with
	 */
	void clearViewport(const glm::vec4& color);

	/**
	 * \brief Clear the viewport depth buffer with the given depth value
	 * \param depth - depth value to clear the viewport with
	 */
	void clearViewport(const float depth);

	/**
	 * \brief Makes the window context current on the calling thread
	 */
	void makeWindowContextcurrent();

	/**
	 * \brief Swaps the front and back buffers
	 */
	void swapBuffers();

	/**
	 * \brief Returns the length of the last frame in seconds
	 * \return The length of the last frame in seconds
	 */
	double getLastFrameTime() const;

	/**
	 * \brief Sets the viewport to the given values
	 * 
	 * \param x - x position of the viewport
	 * \param y - y position of the viewport
	 * \param width - width of the viewport
	 * \param height - height of the viewport
	 */
	void setViewport(int x, int y, int width, int height);

	void bindDefaultFramebuffer();

	unsigned int getScreenWidth() const;
	unsigned int getScreenHeight() const;
	
	/**
	 * \brief Returns the window associated with this render engine
	 * 
	 * \return The window associated with this render engine
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
