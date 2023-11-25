#pragma once

#include "../glad/glad.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <unordered_map>
#include <set>
#include <optional>
#include <functional>

#include "engineGeometry.h"
#include "callback.hpp"


class RenderEngine {
private:
	static std::unordered_map<GLFWwindow*, RenderEngine*> engineWindowPairs;
	
	static RenderEngine* getEngine(GLFWwindow* window);

	static void framebufferSizeChangedCallback(GLFWwindow* window, int width, int height);

	static void mouseCallbackFun(GLFWwindow* window, double xPos, double yPos);
	static void scrollCallbackFun(GLFWwindow* window, double xoffset, double yoffset);
	static void mouseButtonCallbackFun(GLFWwindow* window, int button, int action, int mods);
	static void keyCallbackFun(GLFWwindow* window, int key, int scancode, int action, int mode);

public:
	RenderEngine(int screenWidth, int screenHeight, std::string name);

	~RenderEngine();

	/*
	* Also binds texture0-texture9 to sampler 0-9
	*/
	unsigned int createGPUProgram(std::string vertexShaderName, std::string fragmentShaderName);
	void deleteGPUProgram(unsigned int id);

	void activateGPUProgram(unsigned int program);

	void drawGeometryArray(unsigned int geometry, int num = -1) const;

	void renderWireframeOnly(bool enable);

	void setUniformInt(unsigned int program, std::string name, int value);
	void setUniformFloat(unsigned int program, std::string name, float value);
	void setUniformVec2(unsigned int program, std::string name, glm::vec2 value);
	void setUniformVec3(unsigned int program, std::string name, glm::vec3 value);
	void setUniformVec4(unsigned int program, std::string name, glm::vec4 value);
	void setUniformMat4(unsigned int program, std::string name, glm::mat4 value);
	void setUniformSampler(unsigned int program, std::string name, int value);

	void clearViewport(const glm::vec4& color, int bufferBits);

	/*
	* In the vertex shader the input locations are:
	*	0  - vertex position vec3
	*	1  - vertex normal vec3
	*	2  - texture position vec2
	*	10 - object position vec3 (per instance)
	*	11 - object ambient color vec3 (pre instance)
	*/
	unsigned int createGeometryArray(InputGeometryArray& input);
	void updatePosition(unsigned int geometryId, std::vector<glm::vec3>& position, int num = -1);
	void updateColor(unsigned int geometryId, std::vector<glm::vec4>& color, int num = -1);
	void removeGeometryArray(unsigned int geometry);

	unsigned int loadTexture(std::string name, int minSampler, int magSampler, bool repeatingTiles = false);
	/*
	* Creates a render target texture with a framebuffer
	*	If no params are provided, it gets always automatically resized to the viewport size
	* It needs to be rebinded (fbo and texture) after every resize (or every frame)
	*/
	unsigned int createRenderTargetTexture(int width = -1, int height = -1);
	void bindTexture(unsigned int sampler, unsigned int texture);
	void bindRenderTargetTexture(unsigned int sampler, unsigned int id);
	void deleteTexture(unsigned int id);
	void deleteRenderTargetTexture(unsigned int id);

	void makeWindowContextcurrent();
	void setViewport(int x, int y, int width, int height);

	/*
	* If no params are provided, default framebuffer is binded
	*/
	void bindFramebuffer(unsigned int renderTargetTextureId = -1);

	unsigned int getScreenWidth() const;
	unsigned int getScreenHeight() const;
	GLFWwindow* getWindow() const;

	Callback<double, double> mouseCallback;
	Callback<double, double> scrollCallback;
	Callback<int, int, int> mouseButtonCallback;
	Callback<int, int, int, int> keyCallback;

private:
	GLFWwindow* window;

	unsigned int screenWidth;
	unsigned int screenHeight;

	struct InternalGeometryArray {
		unsigned int vaoId, vboId, indexId, posId, colorId;
		bool hasIndexes = false;
		int arraySize;
		int renderAs;
		int renderVertexNum;
	};

	struct RenderTargetTexture {
		bool autoResize;
		int width;
		int height;
		unsigned int fbo;
		unsigned int rbo;
		unsigned int textureId;
	};

	unsigned int activeProgram = 65535;

	std::set<unsigned int> shaderPrograms;
	std::set<unsigned int> textures;
	std::unordered_map<unsigned int, RenderTargetTexture> renderTargetTextures;
	std::unordered_map<unsigned int, InternalGeometryArray> internalGeometryArrayMap;
	unsigned int geometryIdCounter = 0;
	unsigned int renderTargetIdCounter = 0;

	void removeInternalGeometry(InternalGeometryArray& geometry);
};
