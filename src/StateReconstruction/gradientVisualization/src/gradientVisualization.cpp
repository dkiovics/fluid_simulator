#include "gradientVisualization/gradientVisualization.h"

#include "engine/glUtils.hpp"

using namespace diffrender;

GradientVisualization::GradientVisualization(
    std::shared_ptr<renderer::Camera3D> camera,
    std::shared_ptr<renderer::Lights> lights)
{
    arrowShader = std::make_shared<renderer::ShaderProgram>(
        "shaders/utils/arrow.vert", "shaders/utils/arrow.frag");

    arrowGeometry = std::make_unique<renderer::InstancedGeometry>(
        std::make_shared<renderer::Arrow4>(0.1f, 1.2f, 0.6f));

    snapshotSSBO = renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>(1, GL_DYNAMIC_COPY);

    if (camera)
    {
        camera->addProgram({ arrowShader });
        camera->setUniformsForAllPrograms();
    }
    if (lights)
    {
        lights->addProgram({ arrowShader });
        lights->setUniformsForAllPrograms();
    }
}

void GradientVisualization::snapshotGradient(
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> gradient)
{
    if (!gradient)
        return;

    if (snapshotSSBO->getSize() != gradient->getSize())
        snapshotSSBO->setSize(gradient->getSize());

    gradient->copyTo(*snapshotSSBO);
    snapshotValid = true;
}

void GradientVisualization::ensureLocalFramebuffer(renderer::fb_ptr canvasFramebuffer)
{
    const glm::ivec2 size = canvasFramebuffer->getSize();
    const auto& canvasColors = canvasFramebuffer->getColorAttachments();

    if (!localDepthTexture)
    {
        localDepthTexture = std::make_shared<renderer::RenderTargetTexture>(
            size.x, size.y, GL_NEAREST, GL_NEAREST,
            GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
    }
    else if (localDepthTexture->getSize() != size)
    {
        localDepthTexture->resizeTexture(size.x, size.y);
    }

    if (!localFramebuffer)
    {
        localFramebuffer = std::make_unique<renderer::Framebuffer>(
            canvasColors, localDepthTexture, false);
    }
    else
    {
        // Color attachment points at the caller's canvas color; refresh each
        // frame in case the caller passes a different canvas or its color
        // attachment has been swapped out.
        localFramebuffer->setColorAttachments(canvasColors);
        if (localFramebuffer->getSize() != size)
            localFramebuffer->setSize(size);
    }
}

void GradientVisualization::render(
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> positions,
    renderer::fb_ptr framebuffer,
    float densityThreshold)
{
    if (!snapshotValid || !positions || !framebuffer)
        return;

    if (positions->getSize() != snapshotSSBO->getSize())
        return;

    ensureLocalFramebuffer(framebuffer);

    localFramebuffer->bind();
    // Fresh depth each frame so arrow ordering is determined purely by the
    // arrows themselves (the canvas's own depth is left undefined by the
    // fullscreen-quad copy that ends Visuals3D::render).
    localFramebuffer->clearDepthAttachment(1.0f);
    renderer::enableDepthTest(true);

    arrowShader->activate();
    (*arrowShader)["densityThreshold"] = densityThreshold;

    positions->bindBuffer(0);
    snapshotSSBO->bindBuffer(1);

    arrowGeometry->setInstanceNum(positions->getSize());
    arrowGeometry->draw();
}
