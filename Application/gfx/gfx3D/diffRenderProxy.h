#pragma once

#include "renderer3DInterface.h"
#include <memory>

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
	std::shared_ptr<Renderer3DInterface> renderer3D;
};

} // namespace gfx3D