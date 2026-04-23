#pragma once

#include <memory>
#include "engine/framebuffer.h"
#include "engine/shaderProgram.h"
#include "engine/texture.h"
#include "geometries/basicGeometries.h"

namespace renderer
{

class GenericGpuPrograms
{
public:
    static const GenericGpuPrograms& instance();

    void copyTextureToFramebuffer(const std::shared_ptr<Texture>& source,
                                  glm::ivec2 textureSize,
                                  const std::shared_ptr<Framebuffer>& destination) const;

    void showTextureOnScreen(const std::shared_ptr<Texture>& texture, glm::ivec2 textureSize, glm::ivec2 offsetOnScreen = glm::ivec2(0, 0)) const;

private:
    static std::unique_ptr<GenericGpuPrograms> instancePtr;

    GenericGpuPrograms();

    std::unique_ptr<renderer::ShaderProgram> fullScreenShader;
    std::unique_ptr<renderer::Square> fullScreenQuad;
};

} // namespace renderer
