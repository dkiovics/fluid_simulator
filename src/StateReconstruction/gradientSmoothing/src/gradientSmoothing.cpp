#include "gradientSmoothing/gradientSmoothing.h"

#include <stdexcept>

using namespace diffrender;

GradientSmoothing::GradientSmoothing(float smoothingSphereR, glm::vec3 particleBox)
    : particleBox(particleBox), smoothingSphereR(smoothingSphereR),
      particleCellCount(glm::max(glm::ivec3(glm::ceil(particleBox / smoothingSphereR)), glm::ivec3(1)))
{
    buildSpatialHashCompute =
        renderer::make_compute("shaders/stateReconstruction/gradientSmoothing/buildSpatialHash.comp");
    smoothGradientCompute =
        renderer::make_compute("shaders/stateReconstruction/gradientSmoothing/smoothGradient.comp");
    gradientCopyCompute =
        renderer::make_compute("shaders/stateReconstruction/gradientSmoothing/gradientCopy.comp");

    (*buildSpatialHashCompute)["smoothingSphereR"] = smoothingSphereR;
    (*buildSpatialHashCompute)["particleCellCount"] = particleCellCount;
    (*smoothGradientCompute)["smoothingSphereR"] = smoothingSphereR;
    (*smoothGradientCompute)["particleCellCount"] = particleCellCount;

    const size_t cellCount =
        size_t(particleCellCount.x) * size_t(particleCellCount.y) * size_t(particleCellCount.z);
    particleIndexListSSBO = renderer::make_ssbo<ParticleIndexList>((unsigned int) cellCount, GL_DYNAMIC_COPY);
    smoothedGradientTmpSSBO = renderer::make_ssbo<glm::vec4>(1, GL_DYNAMIC_COPY);
}

void GradientSmoothing::smoothGradient(
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> positions,
    renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> gradient)
{
    if (!positions || !gradient)
        throw std::runtime_error("GradientSmoothing::smoothGradient: null buffer");
    if (positions->getSize() != gradient->getSize())
        throw std::runtime_error("GradientSmoothing::smoothGradient: position/gradient size mismatch");

    const unsigned int particleCount = positions->getSize();
    if (particleCount == 0)
        return;

    if (smoothedGradientTmpSSBO->getSize() != particleCount)
        smoothedGradientTmpSSBO->setSize(particleCount);

    // Clear per-cell indexCount (and the indices arrays, harmlessly) to 0 before
    // the GPU-side hash build atomically increments them. Both barrier bits are
    // needed: BUFFER_UPDATE for the clear (a glClearNamedBufferData call) and
    // SHADER_STORAGE for the upcoming shader reads.
    particleIndexListSSBO->fillWithZeros();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

    const unsigned int dispatch = (particleCount + 255) / 256;

    // 1. Build spatial hash: one thread per particle, atomicAdd to claim a slot.
    positions->bindBuffer(0);
    particleIndexListSSBO->bindBuffer(1);
    buildSpatialHashCompute->dispatchCompute(dispatch, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 2. Smooth: per particle, walk 3x3x3 neighbour cells, write to tmp.
    positions->bindBuffer(0);
    particleIndexListSSBO->bindBuffer(1);
    gradient->bindBuffer(2);
    smoothedGradientTmpSSBO->bindBuffer(3);
    smoothGradientCompute->dispatchCompute(dispatch, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 3. Copy tmp xyz back into the in-place gradient buffer, preserving its
    //    .w and the velocity vec4.
    gradient->bindBuffer(2);
    smoothedGradientTmpSSBO->bindBuffer(3);
    gradientCopyCompute->dispatchCompute(dispatch, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
