#pragma once

#include <glad.h>
#include <glm/glm.hpp>

namespace renderer
{

inline void renderWireframeOnly(bool enable)
{
	if (enable)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

inline void enableDepthTest(bool enable)
{
	if (enable)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

inline void clearViewport(const glm::vec4& color, const float depth)
{
	glClearColor(color.r, color.g, color.b, color.a);
	glClearDepth(depth);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void clearViewport(const glm::vec4& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
	glClear(GL_COLOR_BUFFER_BIT);
}

inline void clearViewport(const float depth)
{
	glClearDepth(depth);
	glClear(GL_DEPTH_BUFFER_BIT);
}

inline void setViewport(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}

inline void bindDefaultFramebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


} // namespace renderer
