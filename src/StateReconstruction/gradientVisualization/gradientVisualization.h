#pragma once

#include <memory>
#include "compute/storageBuffer.h"
#include "engine/framebuffer.h"
#include "engine/shaderProgram.h"
#include "engine/texture.h"
#include "engineUtils/camera3D.hpp"
#include "engineUtils/lights.hpp"
#include "geometries/basicGeometries.h"
#include "manager/simulationManager.h"

namespace diffrender
{

/**
 * Draws one arrow per particle, oriented along the negative position-gradient
 * (so arrows point in the direction the particle "wants" to move to reduce the
 * loss). The gradient that was most recently computed is snapshotted so the
 * visualisation remains valid even when state reconstruction is paused.
 *
 * Reads positions and gradients from two separate SSBOs (matching the rest of
 * the current state-reconstruction architecture); no packed combined buffer.
 *
 * Owns a local depth attachment so arrows can be depth-tested against each
 * other (the canvas's own depth attachment, after Visuals3D::render, is left
 * in an undefined state by the fullscreen-quad copy and is therefore not safe
 * to use here). The local depth attachment is cleared each frame; arrows
 * occlude other arrows but not the rendered scene.
 */
class GradientVisualization
{
public:
    GradientVisualization(
        std::shared_ptr<renderer::Camera3D> camera,
        std::shared_ptr<renderer::Lights> lights);

    /// Copy the latest gradient into the snapshot buffer. Should be called when
    /// the gradient is in a fully consistent state (post-correctGradient and
    /// post-smoothing if smoothing is enabled).
    void snapshotGradient(renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> gradient);

    bool hasValidSnapshot() const { return snapshotValid; }

    /// Draw arrows over the contents of `framebuffer`.
    /// `positions` provides per-particle positions and density.
    void render(
        renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> positions,
        renderer::fb_ptr framebuffer,
        float densityThreshold);

private:
    std::shared_ptr<renderer::GpuProgram> arrowShader;
    std::unique_ptr<renderer::InstancedGeometry> arrowGeometry;
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> snapshotSSBO;
    bool snapshotValid = false;

    // Local depth attachment + a framebuffer wrapper. The wrapper's color
    // attachment is swapped to point at the caller's canvas color attachment
    // each render(); the depth attachment is ours, cleared every frame.
    std::shared_ptr<renderer::RenderTargetTexture> localDepthTexture;
    std::unique_ptr<renderer::Framebuffer> localFramebuffer;

    void ensureLocalFramebuffer(renderer::fb_ptr canvasFramebuffer);
};

} // namespace diffrender
