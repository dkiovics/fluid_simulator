#pragma once

#include "../engine/texture.h"

namespace renderer
{

/**
 * \brief Same as RenderTargetTexture but with the difference that it is used for compute shaders, it also includes an Image2D sampler.
 */
class ComputeTexture : public RenderTargetTexture
{
public:
	/**
	 * \brief Creates a new render target texture with the given dimensions.
	 *
	 * \param width - the width of the texture
	 * \param height - the height of the texture
	 * \param access - the access of the texture. Possible values are GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE.
	 * \param minSampler, magSampler - the sampler parameters for the texture.
	 *		Possible values are GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST,
	 *		GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
	 * \param internalFormat - the internal format of the texture. Possible values are GL_RGBA32F, GL_RGBA32I, GL_RGBA32UI etc.
	 * \param format - the format of the texture. Possible values are GL_RGBA, GL_RGB, GL_RED, GL_RG etc.
	 * \param dataType - the data type of the texture. Possible values are GL_FLOAT, GL_INT, GL_UNSIGNED_INT etc.
	 * \param imageMipmapLevel - the mipmap level of the texture that is used as an image2D sampler in the compute shader.
	 */
	ComputeTexture(int width, int height, GLenum access, GLint minSampler = GL_NEAREST, GLint magSampler = GL_NEAREST,
		GLint internalFormat = GL_RGBA32F, GLint format = GL_RGBA, GLint dataType = GL_FLOAT, int imageMipmapLevel = 0);

	ComputeTexture(const ComputeTexture&) = delete;
	ComputeTexture& operator=(const ComputeTexture&) = delete;
	ComputeTexture(ComputeTexture&&) = delete;
	ComputeTexture& operator=(ComputeTexture&&) = delete;

	GLuint getImageSampler() const;

	~ComputeTexture();

private:
	static std::map<GLFWwindow*, std::set<GLint>> availableImageSamplersPerContext;
	static std::mutex availableImageSamplersMutex;

	GLuint imageSampler;
};

}
