#include "texture.h"

#include <stdexcept>
#include <spdlog/spdlog.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

std::map<GLFWwindow*, std::set<GLint>> renderer::Texture::availableTexSamplersPerContext;
std::mutex renderer::Texture::availableTexSamplersMutex;

renderer::Texture::Texture()
{
	{
		std::scoped_lock lock(availableTexSamplersMutex);
		auto context = glfwGetCurrentContext();
		if (availableTexSamplersPerContext.contains(context))
		{
			if (availableTexSamplersPerContext[context].empty())
				throw std::runtime_error("No more texture samplers available for this context");
		}
		else
		{
			availableTexSamplersPerContext[context] = {};
			int maxTexUnits;
			glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
			for (int i = 0; i < maxTexUnits; i++)
				availableTexSamplersPerContext[context].insert(i);
		}
		texSampler = *availableTexSamplersPerContext[context].begin();
		availableTexSamplersPerContext[context].erase(texSampler);
	}
	glActiveTexture(GL_TEXTURE0 + texSampler);
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	spdlog::debug("Texture created with id: {}", textureId);
}

GLuint renderer::Texture::getTextureId() const
{
	return textureId;
}

GLuint renderer::Texture::getTexSampler() const
{
	return texSampler;
}

renderer::Texture::~Texture()
{
	glDeleteTextures(1, &textureId);
	std::scoped_lock lock(availableTexSamplersMutex);
	auto context = glfwGetCurrentContext();
	availableTexSamplersPerContext[context].insert(texSampler);
	spdlog::debug("Texture deleted with id: {}", textureId);
}

renderer::ColorTexture::ColorTexture(const std::string& textureFileName, GLint minSampler, GLint magSampler, bool tiling) : Texture()
{
	if (tiling)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minSampler);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magSampler);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(textureFileName.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
		throw std::runtime_error("Failed to read image file: " + textureFileName);
	stbi_image_free(data);

	spdlog::debug("Color texture created for file: {}", textureFileName);
}

renderer::RenderTargetTexture::RenderTargetTexture(int width, int height, GLint minSampler, GLint magSampler,
	GLint internalFormat, GLint format, GLint dataType) : Texture()
{
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minSampler);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magSampler);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

renderer::RenderTargetTexture::RenderTargetTexture(GLint internalFormat, GLint format, GLint dataType) : Texture()
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, 0, 0, 0, format, dataType, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
}
