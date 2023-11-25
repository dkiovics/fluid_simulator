#include "renderEngine.h"

#include <stdexcept>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


std::unordered_map<GLFWwindow*, RenderEngine*> RenderEngine::engineWindowPairs;

void RenderEngine::framebufferSizeChangedCallback(GLFWwindow* window, int width, int height) {
	auto tmp = getEngine(window);
	if (!tmp)
		return;
	RenderEngine& engine = *tmp;

	glfwMakeContextCurrent(window);
	glViewport(0, 0, width, height);
	engine.screenWidth = width;
	engine.screenHeight = height;
	for (auto& t : engine.renderTargetTextures) {
		if (t.second.autoResize) {
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
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderEngine::mouseCallbackFun(GLFWwindow* window, double x, double y) {
	auto tmp = getEngine(window);
	if (tmp)
		tmp->mouseCallback.triggerCallback(x, y);
}

void RenderEngine::scrollCallbackFun(GLFWwindow* window, double xOffset, double yOffset) {
	auto tmp = getEngine(window);
	if (tmp)
		tmp->scrollCallback.triggerCallback(xOffset, yOffset);
}

void RenderEngine::mouseButtonCallbackFun(GLFWwindow* window, int button, int action, int mods) {
	auto tmp = getEngine(window);
	if (tmp)
		tmp->mouseButtonCallback.triggerCallback(button, action, mods);
}

void RenderEngine::keyCallbackFun(GLFWwindow* window, int key, int scancode, int action, int mode) {
	auto tmp = getEngine(window);
	if (tmp)
		tmp->keyCallback.triggerCallback(key, scancode, action, mode);
}

RenderEngine* RenderEngine::getEngine(GLFWwindow* window) {
	auto pair = engineWindowPairs.find(window);
	if (pair == engineWindowPairs.end())
		return nullptr;
	return pair->second;
}

RenderEngine::RenderEngine(int screenWidth, int screenHeight, std::string name) : screenWidth(screenWidth), screenHeight(screenHeight) {
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

RenderEngine::~RenderEngine() {
	for (auto& p : internalGeometryArrayMap) {
		removeInternalGeometry(p.second);
	}
	for (auto p : shaderPrograms) {
		glDeleteProgram(p);
	}
	for(auto p : textures) {
		glDeleteTextures(1, &p);
	}
	for (auto& p : renderTargetTextures) {
		glDeleteTextures(1, &p.second.textureId);
		glDeleteRenderbuffers(1, &p.second.rbo);
		glDeleteFramebuffers(1, &p.second.fbo);
	}
	glfwMakeContextCurrent(window);
	glfwDestroyWindow(window);
#ifdef _DEBUG
	std::cout << "RenderEngine destructor called" << std::endl;
#endif
}

unsigned int RenderEngine::createGPUProgram(std::string vertexShaderName, std::string fragmentShaderName) {
	std::ifstream vertexShaderFile(vertexShaderName);
	if (!vertexShaderFile)
		throw std::runtime_error("Vertex shader not found");
	std::ostringstream vertexStream;
	vertexStream << vertexShaderFile.rdbuf();
	std::string vertexShaderSource = vertexStream.str();
	const char* tmp = vertexShaderSource.c_str();
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &tmp, NULL);
	glCompileShader(vertexShader);
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		throw std::runtime_error(std::string("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n") + infoLog);
	}

	std::ifstream fragmentShaderFile(fragmentShaderName);
	if (!fragmentShaderFile)
		throw std::runtime_error("Fragment shader not found");
	std::ostringstream fragmentStream;
	fragmentStream << fragmentShaderFile.rdbuf();
	std::string fragmentShaderSource = fragmentStream.str();
	tmp = fragmentShaderSource.c_str();
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &tmp, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		throw std::runtime_error(std::string("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n") + infoLog);
	}
	
	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		throw std::runtime_error(std::string("ERROR::SHADER::PROGRAM::LINKING_FAILED\n") + infoLog);
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	shaderPrograms.insert(shaderProgram);

	activateGPUProgram(shaderProgram);

	for (int p = 0; p < 10; p++) {
		int id = glGetUniformLocation(shaderProgram, (std::string("texture") + std::to_string(p)).c_str());
		if (id != -1)
			glUniform1i(id, p);
	}

	return shaderProgram;
}

void RenderEngine::deleteGPUProgram(unsigned int id) {
	auto tmp = shaderPrograms.find(id);
	if(tmp == shaderPrograms.end())
		throw std::runtime_error("Deletable GPU program not found");
	glDeleteProgram(id);
}

void RenderEngine::activateGPUProgram(unsigned int program) {
	if (activeProgram == program)
		return;
	if (shaderPrograms.find(program) == shaderPrograms.end())
		throw std::runtime_error("GPU program not found");
	glUseProgram(program);
	activeProgram = program;
}

void RenderEngine::drawGeometryArray(unsigned int geometry, int num) const {
	auto result = internalGeometryArrayMap.find(geometry);
	if (result == internalGeometryArrayMap.end())
		throw std::runtime_error("Geometry not found");
	auto& tmp = result->second;
	glBindVertexArray(tmp.vaoId);
	if (num == -1)
		num = tmp.arraySize;
	if(tmp.hasIndexes)
		glDrawElementsInstanced(tmp.renderAs, tmp.renderVertexNum, GL_UNSIGNED_INT, 0, num);
	else
		glDrawArraysInstanced(tmp.renderAs, 0, tmp.renderVertexNum, num);
}

void RenderEngine::renderWireframeOnly(bool enable) {
	if(enable)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

static int uniformWarning(int program, const char* name) {
	int location = glGetUniformLocation(program, name);
	if (location == -1)
		std::cout << "WARNING: could not find uniform named " << name << std::endl;
	return location;
}

void RenderEngine::setUniformInt(unsigned int program, std::string name, int value) {
	activateGPUProgram(program);
	glUniform1i(uniformWarning(program, name.c_str()), value);
}

void RenderEngine::setUniformFloat(unsigned int program, std::string name, float value) {
	activateGPUProgram(program);
	glUniform1f(uniformWarning(program, name.c_str()), value);
}

void RenderEngine::setUniformVec2(unsigned int program, std::string name, glm::vec2 value) {
	activateGPUProgram(program);
	glUniform2fv(uniformWarning(program, name.c_str()), 1, &value[0]);
}

void RenderEngine::setUniformVec3(unsigned int program, std::string name, glm::vec3 value) {
	activateGPUProgram(program);
	glUniform3fv(uniformWarning(program, name.c_str()), 1, &value[0]);
}

void RenderEngine::setUniformVec4(unsigned int program, std::string name, glm::vec4 value) {
	activateGPUProgram(program);
	glUniform4fv(uniformWarning(program, name.c_str()), 1, &value[0]);
}

void RenderEngine::setUniformMat4(unsigned int program, std::string name, glm::mat4 value) {
	activateGPUProgram(program);
	glUniformMatrix4fv(uniformWarning(program, name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void RenderEngine::setUniformSampler(unsigned int program, std::string name, int value) {
	activateGPUProgram(program);
	glUniform1i(uniformWarning(program, name.c_str()), value);
}

void RenderEngine::clearViewport(const glm::vec4& color, int bufferBits) {
	glfwMakeContextCurrent(window);
	glClearColor(color.x, color.y, color.z, color.w);
	glClear(bufferBits);
}

unsigned int RenderEngine::createGeometryArray(InputGeometryArray& input) {
	if (input.arraySize != input.ambientColor.size() || input.arraySize != input.position.size())
		throw std::runtime_error("Failed to create geometry array: inconsistent array size");

	geometryIdCounter++;

	InternalGeometryArray internalGeometry;

	internalGeometry.arraySize = input.arraySize;
	internalGeometry.renderAs = input.renderAs;
	internalGeometry.renderVertexNum = input.hasIndexes ? input.indexes.size() : input.vertexes.size();

	glGenVertexArrays(1, &internalGeometry.vaoId);
	glBindVertexArray(internalGeometry.vaoId);

	glGenBuffers(1, &internalGeometry.vboId);
	glBindBuffer(GL_ARRAY_BUFFER, internalGeometry.vboId);
	glBufferData(GL_ARRAY_BUFFER, input.vertexes.size() * sizeof(Vertex), input.vertexes.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texture));

	if (internalGeometry.hasIndexes = input.hasIndexes) {
		glGenBuffers(1, &internalGeometry.indexId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, internalGeometry.indexId);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, input.indexes.size() * sizeof(unsigned int), input.indexes.data(), GL_STATIC_DRAW);
	}

	glGenBuffers(1, &internalGeometry.posId);
	glBindBuffer(GL_ARRAY_BUFFER, internalGeometry.posId);
	glBufferData(GL_ARRAY_BUFFER, input.position.size() * sizeof(glm::vec3), input.position.data(), GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(10);
	glVertexAttribPointer(10, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glVertexAttribDivisor(10, 1);

	glGenBuffers(1, &internalGeometry.colorId);
	glBindBuffer(GL_ARRAY_BUFFER, internalGeometry.colorId);
	glBufferData(GL_ARRAY_BUFFER, input.ambientColor.size() * sizeof(glm::vec4), input.ambientColor.data(), GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(11);
	glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
	glVertexAttribDivisor(11, 1);

	internalGeometryArrayMap.insert(std::make_pair(geometryIdCounter, internalGeometry));

	return geometryIdCounter;
}

void RenderEngine::updatePosition(unsigned int geometryId, std::vector<glm::vec3>& position, int num) {
	auto result = internalGeometryArrayMap.find(geometryId);
	if (result == internalGeometryArrayMap.end())
		throw std::runtime_error("Geometry not found");
	auto& tmp = result->second;
	if(tmp.arraySize != position.size())
		throw std::runtime_error("Geometry input update position array size is incorrect");
	if (num == -1)
		num = position.size();
	glBindVertexArray(tmp.vaoId);
	glBindBuffer(GL_ARRAY_BUFFER, tmp.posId);
	glBufferSubData(GL_ARRAY_BUFFER, 0, num * sizeof(glm::vec3), position.data());
}

void RenderEngine::updateColor(unsigned int geometryId, std::vector<glm::vec4>& color, int num) {
	auto result = internalGeometryArrayMap.find(geometryId);
	if (result == internalGeometryArrayMap.end())
		throw std::runtime_error("Geometry not found");
	auto& tmp = result->second;
	if (tmp.arraySize != color.size())
		throw std::runtime_error("Geometry input update color array size is incorrect");
	if (num == -1)
		num = color.size();
	glBindVertexArray(tmp.vaoId);
	glBindBuffer(GL_ARRAY_BUFFER, tmp.colorId);
	glBufferSubData(GL_ARRAY_BUFFER, 0, num * sizeof(glm::vec4), color.data());
}

void RenderEngine::removeGeometryArray(unsigned int geometry) {
	auto result = internalGeometryArrayMap.find(geometry);
	if (result != internalGeometryArrayMap.end()) {
		removeInternalGeometry(result->second);
		internalGeometryArrayMap.erase(geometry);
	}
}

unsigned int RenderEngine::loadTexture(std::string name, int minSampler, int magSampler, bool repeatingTiles) {
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	if (repeatingTiles) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minSampler);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magSampler);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); 
	unsigned char* data = stbi_load(name.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
		throw std::runtime_error("Failed to read image file: " + name);
	stbi_image_free(data);
	textures.insert(texture);
	return texture;
}

unsigned int RenderEngine::createRenderTargetTexture(int width, int height) {
	RenderTargetTexture t;
	glGenFramebuffers(1, &t.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
	if (width > 0 && height > 0) {
		t.autoResize = false;
		t.width = width;
		t.height = height;
	}
	else {
		t.autoResize = true;
		t.width = getScreenWidth();
		t.height = getScreenHeight();
	}
	glGenTextures(1, &t.textureId);
	glBindTexture(GL_TEXTURE_2D, t.textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t.width, t.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.textureId, 0);

	glGenRenderbuffers(1, &t.rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, t.rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, t.width, t.height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, t.rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("Failed to create framebuffer");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	renderTargetTextures.insert(std::make_pair(++renderTargetIdCounter, t));

	return renderTargetIdCounter;
}

void RenderEngine::bindTexture(unsigned int sampler, unsigned int texture) {
	glActiveTexture(GL_TEXTURE0 + sampler);
	glBindTexture(GL_TEXTURE_2D, texture);
}

void RenderEngine::bindRenderTargetTexture(unsigned int sampler, unsigned int id) {
	auto t = renderTargetTextures.find(id);
	if (t != renderTargetTextures.end()) {
		glActiveTexture(GL_TEXTURE0 + sampler);
		glBindTexture(GL_TEXTURE_2D, t->second.textureId);
	}
}

void RenderEngine::deleteTexture(unsigned int id) {
	auto t = textures.find(id);
	if (t != textures.end()) {
		glDeleteTextures(1, &id);
		textures.erase(id);
	}
}

void RenderEngine::deleteRenderTargetTexture(unsigned int id) {
	auto t = renderTargetTextures.find(id);
	if (t != renderTargetTextures.end()) {
		glDeleteTextures(1, &t->second.textureId);
		glDeleteRenderbuffers(1, &t->second.rbo);
		glDeleteFramebuffers(1, &t->second.fbo);
	}
}

void RenderEngine::makeWindowContextcurrent() {
	glfwMakeContextCurrent(window);
}

void RenderEngine::setViewport(int x, int y, int width, int height) {
	glViewport(x, y, width, height);
}

void RenderEngine::bindFramebuffer(unsigned int renderTargetTextureId) {
	if (renderTargetTextureId == -1) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	else {
		auto t = renderTargetTextures.find(renderTargetTextureId);
		if (t != renderTargetTextures.end()) {
			glBindFramebuffer(GL_FRAMEBUFFER, t->second.fbo);
		}
	}
}

unsigned int RenderEngine::getScreenWidth() const {
	return screenWidth;
}

unsigned int RenderEngine::getScreenHeight() const {
	return screenHeight;
}

GLFWwindow* RenderEngine::getWindow() const {
	return window;
}

void RenderEngine::removeInternalGeometry(InternalGeometryArray& geometry) {
	glDeleteVertexArrays(1, &geometry.vaoId);
	glDeleteBuffers(1, &geometry.vboId);
	if (geometry.hasIndexes)
		glDeleteBuffers(1, &geometry.indexId);
	glDeleteBuffers(1, &geometry.posId);
	glDeleteBuffers(1, &geometry.colorId);
}
