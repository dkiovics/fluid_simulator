#include "renderEngine.h"

#include <stdexcept>
#include <fstream>
#include <sstream>


using namespace renderer;

std::unordered_map<GLFWwindow*, RenderEngine*> RenderEngine::engineWindowPairs;

void RenderEngine::framebufferSizeChangedCallback(GLFWwindow* window, int width, int height)
{
	auto tmp = getEngine(window);
	if (!tmp)
		return;
	RenderEngine& engine = *tmp;

	glfwMakeContextCurrent(window);
	glViewport(0, 0, width, height);
	engine.screenWidth = width;
	engine.screenHeight = height;
	/*for (auto& t : engine.renderTargetTextures)
	{
		if (t.second.autoResize)
		{
			glDeleteTextures(1, &t.second.textureId);
			glDeleteRenderbuffers(1, &t.second.rbo);
			t.second.width = width;
			t.second.height = height;
			glGenTextures(1, &t.second.textureId);
			glBindTexture(GL_TEXTURE_2D, t.second.textureId);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t.second.width, t.second.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.second.textureId, 0);

			glGenRenderbuffers(1, &t.second.rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, t.second.rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, t.second.width, t.second.height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, t.second.rbo);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				throw std::runtime_error("Failed to resize framebuffer");
		}
	}*/
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderEngine::mouseCallbackFun(GLFWwindow* window, double x, double y)
{
	auto tmp = getEngine(window);
	if (tmp)
		tmp->mouseCallback.triggerCallback(x, y);
}

void RenderEngine::scrollCallbackFun(GLFWwindow* window, double xOffset, double yOffset)
{
	auto tmp = getEngine(window);
	if (tmp)
		tmp->scrollCallback.triggerCallback(xOffset, yOffset);
}

void RenderEngine::mouseButtonCallbackFun(GLFWwindow* window, int button, int action, int mods)
{
	auto tmp = getEngine(window);
	if (tmp)
		tmp->mouseButtonCallback.triggerCallback(button, action, mods);
}

void RenderEngine::keyCallbackFun(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	auto tmp = getEngine(window);
	if (tmp)
		tmp->keyCallback.triggerCallback(key, scancode, action, mode);
}

RenderEngine* RenderEngine::getEngine(GLFWwindow* window)
{
	auto pair = engineWindowPairs.find(window);
	if (pair == engineWindowPairs.end())
		return nullptr;
	return pair->second;
}

RenderEngine::RenderEngine(int screenWidth, int screenHeight, std::string name) : screenWidth(screenWidth), screenHeight(screenHeight)
{
	window = glfwCreateWindow(screenWidth, screenHeight, name.c_str(), NULL, NULL);
	if (window == NULL)
		throw std::runtime_error("Failed to create window");
	engineWindowPairs.insert(std::make_pair(window, this));
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
}

RenderEngine::~RenderEngine()
{
	glfwMakeContextCurrent(window);
	glfwDestroyWindow(window);
#ifdef _DEBUG
	std::cout << "RenderEngine destructor called" << std::endl;
#endif
}

void RenderEngine::activateGPUProgram(unsigned int program)
{
	if (activeProgram == program)
		return;
	glUseProgram(program);
	activeProgram = program;
}

void RenderEngine::renderWireframeOnly(bool enable)
{
	if (enable)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void renderer::RenderEngine::enableDepthTest(bool enable)
{
	if (enable)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

void RenderEngine::clearViewport(const glm::vec4& color, int bufferBits)
{
	glfwMakeContextCurrent(window);
	glClearColor(color.x, color.y, color.z, color.w);
	glClear(bufferBits);
}

void RenderEngine::makeWindowContextcurrent()
{
	glfwMakeContextCurrent(window);
}

void RenderEngine::setViewport(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}

void RenderEngine::bindFramebuffer(unsigned int renderTargetTextureId)
{
	if (renderTargetTextureId == -1)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	else
	{
		/*auto t = renderTargetTextures.find(renderTargetTextureId);
		if (t != renderTargetTextures.end())
		{
			glBindFramebuffer(GL_FRAMEBUFFER, t->second.fbo);
		}*/
	}
}

unsigned int RenderEngine::getScreenWidth() const
{
	return screenWidth;
}

unsigned int RenderEngine::getScreenHeight() const
{
	return screenHeight;
}

GLFWwindow* RenderEngine::getWindow() const
{
	return window;
}
