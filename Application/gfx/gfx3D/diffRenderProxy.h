#pragma once

#include "renderer3DInterface.h"
#include <memory>
#include "engine/framebuffer.h"
#include "compute/computeTexture.h"
#include "compute/computeProgram.h"
#include "compute/storageBuffer.h"
#include "geometries/basicGeometries.h"

namespace gfx3D
{

class DiffRendererProxy : public Renderer3DInterface
{
public:
	DiffRendererProxy(std::shared_ptr<Renderer3DInterface> renderer);

	void render(std::shared_ptr<renderer::Framebuffer> framebuffer, const Gfx3DRenderData& data) const override;
	void setConfigData(const ConfigData3D& data) override;

	void show(int screenWidth) override;

private:
	glm::ivec2 prevScreenSize;

	struct TestSboData
	{
		bool color;
		glm::vec4 colorValue;
	};

	std::shared_ptr<Renderer3DInterface> renderer3D;
	renderer::RenderEngine& renderEngine;

	std::shared_ptr<renderer::Framebuffer> computeTestFramebuffer;
	std::shared_ptr<renderer::ComputeTexture> computeTestTexture;
	std::unique_ptr<renderer::ComputeProgram> computeTestProgram;
	std::unique_ptr<renderer::StorageBuffer<TestSboData>> computeTestBuffer;

	std::unique_ptr<renderer::Square> showQuad;
	std::unique_ptr<renderer::ShaderProgram> showProgram;
};

} // namespace gfx3D