#include "computeTexture.h"

using namespace renderer;

std::map<GLFWwindow*, std::set<GLint>> ComputeTexture::availableImageSamplersPerContext;
std::mutex ComputeTexture::availableImageSamplersMutex;

renderer::ComputeTexture::ComputeTexture(int width, int height, GLenum access, GLint minSampler, GLint magSampler,
	GLint internalFormat, GLint format, GLint dataType, int imageMipmapLevel)
	: RenderTargetTexture(width, height, minSampler, magSampler, internalFormat, format, dataType)
{
	{
		std::scoped_lock lock(availableImageSamplersMutex);
		auto context = glfwGetCurrentContext();
		if (availableImageSamplersPerContext.contains(context))
		{
			if (availableImageSamplersPerContext[context].empty())
				throw std::runtime_error("No more image samplers available for this context");
		}
		else
		{
			availableImageSamplersPerContext[context] = {};
			int maxTexUnits;
			glGetIntegerv(GL_MAX_IMAGE_SAMPLES, &maxTexUnits);
			for (int i = 1; i < maxTexUnits; i++)
				availableImageSamplersPerContext[context].insert(i);
		}
		imageSampler = *availableImageSamplersPerContext[context].begin();
		availableImageSamplersPerContext[context].erase(imageSampler);
	}
	glBindImageTexture(imageSampler, getTextureId(), imageMipmapLevel, GL_FALSE, 0, access, internalFormat);
}

GLuint renderer::ComputeTexture::getImageSampler() const
{
	return imageSampler;
}

renderer::ComputeTexture::~ComputeTexture()
{
	std::scoped_lock lock(availableImageSamplersMutex);
	auto context = glfwGetCurrentContext();
	availableImageSamplersPerContext[context].insert(imageSampler);
	glBindImageTexture(imageSampler, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
}
