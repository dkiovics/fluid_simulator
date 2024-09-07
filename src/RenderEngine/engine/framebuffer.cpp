#include "framebuffer.h"
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

using namespace renderer;

renderer::Framebuffer::Framebuffer(
	std::vector<std::shared_ptr<RenderTargetTexture>> colorAttachments, std::shared_ptr<RenderTargetTexture> depthAttachment, bool hasStencil)
	: colorAttachments(colorAttachments), depthAttachment(depthAttachment)
{
	if (!this->colorAttachments.empty())
	{
		size = this->colorAttachments[0]->getSize();
	}
	else if (depthAttachment)
	{
		size = depthAttachment->getSize();
	}
	else
	{
		throw std::runtime_error("Framebuffer must have at least one attachment");
	}

	glGenFramebuffers(1, &framebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

	std::vector<GLenum> drawBuffers;
	for (size_t i = 0; i < this->colorAttachments.size(); i++)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, this->colorAttachments[i]->getTextureId(), 0);
		drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
	}

	if (depthAttachment)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, hasStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_2D, depthAttachment->getTextureId(), 0);
	}

	glDrawBuffers(drawBuffers.size(), drawBuffers.data());

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw std::runtime_error("Framebuffer is not complete");
	}

	spdlog::debug("Framebuffer created with id: {}, size: {}x{}, color attachments: {}, depth attachment: {}",
				framebufferId, size.x, size.y, this->colorAttachments.size(), depthAttachment != nullptr);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

renderer::Framebuffer::Framebuffer(std::vector<std::shared_ptr<RenderTargetTexture>> colorAttachments)
	: colorAttachments(colorAttachments)
{
	if (this->colorAttachments.empty())
	{
		throw std::runtime_error("Framebuffer must have at least one attachment");
	}

	size = this->colorAttachments[0]->getSize();

	glGenFramebuffers(1, &framebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

	std::vector<GLenum> drawBuffers;
	for (size_t i = 0; i < this->colorAttachments.size(); i++)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, this->colorAttachments[i]->getTextureId(), 0);
		drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
	}

	glDrawBuffers(drawBuffers.size(), drawBuffers.data());

	GLuint rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x, size.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	depthStencilRenderbufferId = rbo;

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw std::runtime_error("Framebuffer is not complete");
	}

	spdlog::debug("Framebuffer created with id: {}, size: {}x{}, color attachments: {}, renderbuffer depth/stencil attachement",
		framebufferId, size.x, size.y, this->colorAttachments.size());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderer::Framebuffer::bind() const
{
	for (auto& colorAttachment : colorAttachments)
	{
		if (colorAttachment->getSize() != size)
		{
			throw std::runtime_error("Color attachments must have the same size");
		}
	}
	if (depthAttachment && depthAttachment->getSize() != size)
	{
		throw std::runtime_error("Depth attachment must have the same size as the color attachments");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
}

void renderer::Framebuffer::setSize(glm::ivec2 size)
{
	if (size == this->size)
	{
		return;
	}

	for (auto& colorAttachment : colorAttachments)
	{
		colorAttachment->resizeTexture(size.x, size.y);
	}
	if (depthAttachment)
	{
		depthAttachment->resizeTexture(size.x, size.y);
	}

	if (depthStencilRenderbufferId)
	{
		glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRenderbufferId.value());
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, size.x, size.y);
	}

	spdlog::debug("Framebuffer resized with id: {}, new size: {}x{}", framebufferId, size.x, size.y);

	this->size = size;
}

std::vector<std::shared_ptr<RenderTargetTexture>> renderer::Framebuffer::getColorAttachments() const
{
	return colorAttachments;
}

std::shared_ptr<RenderTargetTexture> renderer::Framebuffer::getDepthAttachment() const
{
	return depthAttachment;
}

glm::ivec2 renderer::Framebuffer::getSize() const
{
	return size;
}

renderer::Framebuffer::~Framebuffer()
{
	glDeleteFramebuffers(1, &framebufferId);
	if (depthStencilRenderbufferId)
	{
		glDeleteRenderbuffers(1, &depthStencilRenderbufferId.value());
		spdlog::debug("Framebuffer deleted with id: {}, depth/stencil renderbuffer deleted with id: {}",
			framebufferId, depthStencilRenderbufferId.value());
	}
	else
	{
		spdlog::debug("Framebuffer deleted with id: {}", framebufferId);
	}
}

