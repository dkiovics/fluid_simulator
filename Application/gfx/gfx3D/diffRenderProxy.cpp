#include "diffRenderProxy.h"

using namespace gfx3D;

DiffRendererProxy::DiffRendererProxy(std::shared_ptr<Renderer3DInterface> renderer3D)
	:renderer3D(renderer3D) { }

void DiffRendererProxy::render(std::shared_ptr<renderer::Framebuffer> framebuffer, const Gfx3DRenderData& data) const
{
	renderer3D->render(framebuffer, data);
}

void DiffRendererProxy::setConfigData(const ConfigData3D& data)
{
	renderer3D->setConfigData(data);
}

void DiffRendererProxy::show(int screenWidth)
{
	ImGui::SeparatorText("DiffRendererProxy");
	ParamLineCollection::show(screenWidth);
	ImGui::SeparatorText("Renderer 3D");
	renderer3D->show(screenWidth);
}
