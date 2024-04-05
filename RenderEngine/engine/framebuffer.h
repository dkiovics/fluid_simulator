#pragma once

#include "../glad/glad.h"
#include "texture.h"
#include <vector>
#include <memory>
#include <optional>

namespace renderer
{

class Framebuffer
{
public:
	/**
	 * \brief Constructor for the Framebuffer class
	 * \param colorAttachments - The color attachments of the framebuffer, if empty, the framebuffer will not have any color attachments
	 * \param depthAttachment - The depth attachment of the framebuffer, if nullptr, the framebuffer will not have a depth attachment
	 */
	Framebuffer(std::vector<std::shared_ptr<RenderTargetTexture>> colorAttachments, std::shared_ptr<RenderTargetTexture> depthAttachment);

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

private:
	GLuint framebufferId = 0;
	std::optional<GLuint> depthStencilRenderbufferId;
	std::vector<std::shared_ptr<RenderTargetTexture>> colorAttachments;
	std::shared_ptr<RenderTargetTexture> depthAttachment;
	glm::ivec2 size;
};


} // namespace renderer


