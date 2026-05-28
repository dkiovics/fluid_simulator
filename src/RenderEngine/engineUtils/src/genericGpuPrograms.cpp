#include "engineUtils/genericGpuPrograms.h"
#include "engine/glUtils.hpp"

using namespace renderer;

std::unique_ptr<GenericGpuPrograms> GenericGpuPrograms::instancePtr = nullptr;

const GenericGpuPrograms& GenericGpuPrograms::instance()
{
    if (!instancePtr)
        instancePtr = std::unique_ptr<GenericGpuPrograms>(new GenericGpuPrograms());
    return *instancePtr;
}

GenericGpuPrograms::GenericGpuPrograms()
{
    fullScreenShader =
        std::make_unique<ShaderProgram>("shaders/utils/fullScreen.vert", "shaders/utils/fullScreen.frag");
    fullScreenQuad = std::make_unique<Square>();
}

void GenericGpuPrograms::copyTextureToFramebuffer(const std::shared_ptr<Texture>& source,
                                                  glm::ivec2 textureSize,
                                                  const std::shared_ptr<Framebuffer>& destination) const
{
    renderer::setViewport(0, 0, destination->getSize().x, destination->getSize().y);
    destination->bind();
    renderer::enableDepthTest(false);
    fullScreenShader->activate();
    (*fullScreenShader)["colorTexture"] = *source;
    fullScreenQuad->draw();
}

void GenericGpuPrograms::showTextureOnScreen(const std::shared_ptr<Texture>& texture, glm::ivec2 textureSize, glm::ivec2 offsetOnScreen) const
{
    renderer::bindDefaultFramebuffer();
    renderer::setViewport(offsetOnScreen.x, offsetOnScreen.y, textureSize.x, textureSize.y);
    renderer::enableDepthTest(false);
    fullScreenShader->activate();
    (*fullScreenShader)["colorTexture"] = *texture;
    fullScreenQuad->draw();
}
