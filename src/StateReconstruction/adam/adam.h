#pragma once

#include <compute/computeProgram.h>
#include <compute/storageBuffer.h>
#include "manager/simulationManager.h"

namespace diffrender
{

class AdamOptimizer
{
public:
    AdamOptimizer();

    AdamOptimizer(const AdamOptimizer& other) = delete;
    AdamOptimizer(AdamOptimizer&& other) noexcept = delete;
    AdamOptimizer& operator=(const AdamOptimizer& other) = delete;
    AdamOptimizer& operator=(AdamOptimizer&& other) noexcept = delete;

    void init(size_t particleNum);

    void setFluidBoxBounds(glm::vec3 lowerBound, glm::vec3 upperBound);

    void optimize(const renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
                  const renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> g);

    renderer::ssbo_ptr<float> getLastParticleMovementAbs() const;

private:
    float t = 0.0f;

    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> mVec;
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> vVec;
    renderer::ssbo_ptr<float> particleMovementAbs;

    renderer::compute_ptr adamOptimizerCompute;
};

} // namespace diffrender
