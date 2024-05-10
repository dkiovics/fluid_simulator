#pragma once

#include <string>
#include "..\glad\glad.h"
#include <GLFW\glfw3.h>
#include <set>
#include <map>
#include <mutex>
#include "renderEngine.h"

namespace renderer
{

class Texture
{
public:
	Texture();

	Texture& operator=(const Texture&) = delete;
	Texture(const Texture&) = delete;
	Texture& operator=(Texture&&) = delete;
	Texture(Texture&&) = delete;

	/**
	 * \brief Returns the texture id on the GPU.
	 * \return - the texture id on the GPU
	 */
	GLuint getTextureId() const;

	/**
	 * \brief Returns the texture sampler id on the GPU that is used for this texture.
	 * \return - the texture sampler id on the GPU
	 */
	GLuint getTexSampler() const;

	void generateMipmaps() const;

	virtual ~Texture();

protected:
	/**
	 * Mock function to make the class abstract.
	 */
	virtual void pureVirtual() = 0;

	/**
	* \brief Binds the texture to the current context.
	*/
	void bindTexture() const;

	/**
	* \brief Unbinds the texture from the current context.
	*/
	void unbindTexture() const;

private:
	static std::map<GLFWwindow*, std::set<GLint>> availableTexSamplersPerContext;
	static std::mutex availableTexSamplersMutex;

	GLuint textureId;
	GLuint texSampler;
};


class ColorTexture : public Texture
{
public:
	/**
	 * \brief Creates a new color texture from the given image file.
	 * 
	 * \param textureFileName - the name of the image file
	 * \param minSampler, magSampler - the sampler parameters for the texture. 
	 *		Possible values are GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, 
	 *		GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
	 * \param tiling - if true, the texture will be repeated when the texture coordinates are outside the range [0, 1]
	 */
	ColorTexture(const std::string& textureFileName, GLint minSampler, GLint magSampler, bool tiling = true);

protected:
	/**
	 * Mock override function to make the class instantiable.
	 */
	virtual void pureVirtual() override {}

};

class RenderTargetTexture : public Texture
{
public:
	/**
	 * \brief Creates a new render target texture with the given dimensions.
	 * 
	 * \param width - the width of the texture
	 * \param height - the height of the texture
	 * \param minSampler, magSampler - the sampler parameters for the texture. 
	 *		Possible values are GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, 
	 *		GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
	 * \param internalFormat - the internal format of the texture. Possible values are GL_RGBA32F, GL_RGBA32I, GL_RGBA32UI etc.
	 * \param format - the format of the texture. Possible values are GL_RGBA, GL_RGB, GL_RED, GL_RG etc.
	 * \param dataType - the data type of the texture. Possible values are GL_FLOAT, GL_INT, GL_UNSIGNED_INT etc.
	 */
	RenderTargetTexture(int width, int height, GLint minSampler = GL_NEAREST, GLint magSampler = GL_NEAREST, 
		GLint internalFormat = GL_RGBA32F, GLint format = GL_RGBA, GLint dataType = GL_FLOAT);

	void resizeTexture(int width, int height);

	glm::ivec2 getSize() const;

protected:
	/**
	 * Mock override function to make the class instantiable.
	 */
	virtual void pureVirtual() override {}

private:
	glm::ivec2 size;
	const GLint minSampler;
	const GLint magSampler;
	const GLint internalFormat;
	const GLint format;
	const GLint dataType;
};

} // namespace renderer


