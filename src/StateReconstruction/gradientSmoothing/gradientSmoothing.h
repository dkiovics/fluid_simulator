#pragma once

#include <array>
#include <glm/glm.hpp>
#include "compute/computeProgram.h"
#include "compute/storageBuffer.h"
#include "manager/simulationManager.h"

namespace diffrender
{

/**
 * Spatial-hash based Gaussian smoothing of a per-particle position-gradient
 * buffer. Bins particles into a uniform 3D grid (cell size = smoothingSphereR),
 * then for each particle averages its neighbours' gradients with a Gaussian
 * falloff within the sphere of radius smoothingSphereR.
 *
 * The smoothing runs in-place on the gradient buffer (via an internal temp
 * buffer + copy-back) so downstream consumers (Adam) see the smoothed values.
 */
class GradientSmoothing
{
public:
    /// Hard cap on particles tracked per cell. Must match MAX_INDICES in the
    /// buildSpatialHash / smoothGradient shaders.
    static constexpr int MAX_INDICES_PER_CELL = 200;

    GradientSmoothing(float smoothingSphereR, glm::vec3 particleBox);

    /// In-place Gaussian smoothing. `positions` provides the per-particle xyz
    /// positions (in `.posAndDensity.xyz`), `gradient` is the buffer to smooth
    /// in place — its `.posAndDensity.xyz` is replaced with the smoothed value.
    void smoothGradient(
        renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> positions,
        renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> gradient);

    const glm::vec3 particleBox;
    const float smoothingSphereR;

private:
    struct ParticleIndexList
    {
        int indexCount;
        std::array<int, MAX_INDICES_PER_CELL> indices;
    };

    glm::ivec3 particleCellCount;

    renderer::ssbo_ptr<ParticleIndexList> particleIndexListSSBO;
    renderer::ssbo_ptr<glm::vec4> smoothedGradientTmpSSBO;

    renderer::compute_ptr buildSpatialHashCompute;
    renderer::compute_ptr smoothGradientCompute;
    renderer::compute_ptr gradientCopyCompute;
};

} // namespace diffrender
