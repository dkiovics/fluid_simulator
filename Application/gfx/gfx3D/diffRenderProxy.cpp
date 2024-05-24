#include "diffRenderProxy.h"

using namespace gfx3D;

DiffRendererProxy::DiffRendererProxy(std::shared_ptr<Renderer3DInterface> renderer3D)
	:renderer3D(renderer3D), renderEngine(renderer::RenderEngine::getInstance())
{
	computeTestTexture = std::make_shared<renderer::ComputeTexture>(1000, 1000, GL_READ_WRITE);
	computeTestFramebuffer = std::make_shared<renderer::Framebuffer>
		(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>{computeTestTexture});
	computeTestProgram = std::make_unique<renderer::ComputeProgram>("shaders/computeTest.comp");
	(*computeTestProgram)["image"].setImageUnit(*computeTestTexture);
	prevScreenSize = glm::ivec2(1000, 1000);

	computeTestBuffer = std::make_unique<renderer::StorageBuffer<TestSboData>>(1000, GL_STATIC_DRAW);
	computeTestBuffer->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	for (int i = 0; i < 1000; i++)
	{
		(*computeTestBuffer)[i].colorValue[0] = i / float(1000);
		(*computeTestBuffer)[i].colorValue[1] = 1 - i / float(1000);
		(*computeTestBuffer)[i].color = (i / 100) % 2 == 0;
	}
	computeTestBuffer->unmapBuffer();
	computeTestBuffer->bindBuffer(20);

	showQuad = std::make_unique<renderer::Square>();
	showProgram = std::make_unique<renderer::ShaderProgram>("shaders/quad.vert", "shaders/quad.frag");
	(*showProgram)["colorTexture"] = *computeTestTexture;
}

void DiffRendererProxy::render(std::shared_ptr<renderer::Framebuffer> framebuffer, const Gfx3DRenderData& data) const
{
	computeTestFramebuffer->bind();
	renderEngine.setViewport(0, 0, computeTestTexture->getSize().x, computeTestTexture->getSize().x);
	renderEngine.clearViewport(glm::vec4(0, 0, 0, 1), 1.0f);
	renderEngine.enableDepthTest(true);
	renderer3D->render(computeTestFramebuffer, data);

	computeTestProgram->dispatchCompute(computeTestTexture->getSize().x, computeTestTexture->getSize().y, 1);

	framebuffer->bind();
	renderEngine.enableDepthTest(false);
	showProgram->activate();
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	showQuad->draw();
}

void DiffRendererProxy::setConfigData(const ConfigData3D& data)
{
	renderer3D->setConfigData(data);

	if(prevScreenSize != data.screenSize)
	{
		computeTestFramebuffer->setSize(data.screenSize);

		computeTestBuffer->setSize(data.screenSize.y);
		computeTestBuffer->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
		for (int i = 0; i < data.screenSize.y; i++)
		{
			(*computeTestBuffer)[i].colorValue[0] = i / float(data.screenSize.y);
			(*computeTestBuffer)[i].colorValue[1] = 1 - i / float(data.screenSize.y);
			(*computeTestBuffer)[i].color = (i / 100) % 2 == 0;
		}
		computeTestBuffer->unmapBuffer();

		prevScreenSize = data.screenSize;
	}
}

void DiffRendererProxy::show(int screenWidth)
{
	ImGui::SeparatorText("DiffRendererProxy");
	ParamLineCollection::show(screenWidth);
	ImGui::SeparatorText("Renderer 3D");
	renderer3D->show(screenWidth);
}
