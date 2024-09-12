#pragma once

#include <glad.h>
#include "texture.h"
#include <vector>
#include <memory>
#include <optional>
#include <initializer_list>

namespace renderer
{

class Framebuffer
{
public:
	/**
	 * \brief Constructor for the Framebuffer class
	 * \param colorAttachments - The color attachments of the framebuffer, if empty, the framebuffer will not have any color attachments
	 * \param depthAttachment - The depth attachment of the framebuffer, if nullptr, the framebuffer will not have a depth attachment
	 * \param hasStencil - If true, the depthAttachment will be attached as GL_DEPTH_STENCIL_ATTACHMENT
	 */
	Framebuffer(std::vector<std::shared_ptr<RenderTargetTexture>> colorAttachments, 
		std::shared_ptr<RenderTargetTexture> depthAttachment, bool hasStencil = true);

	/**
	* \brief Constructor for the Framebuffer class, creates a framebuffer with a renderbuffer GL_DEPTH_STENCIL_ATTACHMENT
	* \param colorAttachments - The color attachments of the framebuffer, if empty, the framebuffer will not have any color attachments
	*/
	Framebuffer(std::vector<std::shared_ptr<RenderTargetTexture>> colorAttachments);

	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator=(const Framebuffer&) = delete;
	Framebuffer(Framebuffer&&) = delete;
	Framebuffer& operator=(Framebuffer&&) = delete;

	~Framebuffer();

	/**
	 * \brief Binds the framebuffer to the current context. If the framebuffer is incomplete,
	 * or the color attachments and the depth attachment do not have the same size, this function will throw an exception.
	 */
	void bind() const;

	/**
	 * \brief resizes the framebuffer and all its attachments
	 * \param size - The new size of the framebuffer
	 */
	void setSize(glm::ivec2 size);

	/**
	 * \brief Returns the color attachments of the framebuffer
	 * \return The color attachments of the framebuffer
	 */
	std::vector<std::shared_ptr<RenderTargetTexture>> getColorAttachments() const;

	/**
	* \brief Sets the color attachments of the framebuffer
	* \param colorAttachments - The new color attachments of the framebuffer
	*/
	void setColorAttachments(std::vector<std::shared_ptr<RenderTargetTexture>> colorAttachments);

	/**
	* \brief Sets the depth attachment of the framebuffer
	* \param depthAttachment - The new depth attachment of the framebuffer
	*/
	void setDepthAttachment(std::shared_ptr<RenderTargetTexture> depthAttachment);

	/**
	 * \brief Returns the depth attachment of the framebuffer
	 * \return The depth attachment of the framebuffer
	 */
	std::shared_ptr<RenderTargetTexture> getDepthAttachment() const;

	/**
	 * \brief Returns the size of the framebuffer
	 * \return The size of the framebuffer
	 */
	glm::ivec2 getSize() const;

	/**
	 * \brief Converts a list of textures to an array.
	 */
	static std::vector<std::shared_ptr<renderer::RenderTargetTexture>> toArray(std::initializer_list<std::shared_ptr<renderer::RenderTargetTexture>> texture)
	{
		return { texture };
	}

private:
	GLuint framebufferId = 0;
	std::optional<GLuint> depthStencilRenderbufferId;
	std::vector<std::shared_ptr<RenderTargetTexture>> colorAttachments;
	std::shared_ptr<RenderTargetTexture> depthAttachment;
	glm::ivec2 size;
};


} // namespace renderer


