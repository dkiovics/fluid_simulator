#include "dataUtils/dataUtils.h"
#include <cstdlib>
#include "controlRegistry.h"

using namespace diffrender;

MiscDataUtils::MiscDataUtils()
{
    clampPositionsCompute = renderer::make_compute("shaders/stateReconstruction/dataUtils/clampPositions.comp");
}

void MiscDataUtils::clampParticlePositions(renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
                                     glm::vec3 lowerBound,
                                     glm::vec3 upperBound)
{
    data->bindBuffer(0);
    (*clampPositionsCompute)["lowerBound"] = lowerBound;
    (*clampPositionsCompute)["upperBound"] = upperBound;
    clampPositionsCompute->dispatchCompute(data->getSize() / 64 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

DensityControl::DensityControl(std::shared_ptr<genericfsim::manager::SimulationManager> simManager)
    : simManager(simManager)
{
    avgMovementCompute = renderer::make_compute("shaders/stateReconstruction/dataUtils/avgMovement.comp");
    initAvgMovementArray =
        renderer::make_compute("shaders/stateReconstruction/dataUtils/initAvgMovementArray.comp");
    updatePositionsCompute = renderer::make_compute("shaders/stateReconstruction/dataUtils/updatePositions.comp");
}

void DensityControl::init(size_t particleNum)
{
    if (!avgMovement || avgMovement->getSize() != particleNum)
    {
        avgMovement = renderer::make_ssbo<AvgMovement>((unsigned int) particleNum, GL_DYNAMIC_COPY);
    }
    sampleCount = 0;
    avgMovement->bindBuffer(0);
    initAvgMovementArray->dispatchCompute(avgMovement->getSize() / 64 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

bool DensityControl::updateAvgMovement(renderer::ssbo_ptr<float> data)
{
    if (data->getSize() != avgMovement->getSize())
        throw std::runtime_error("DensityControl::updateAvgMovement: data size does not match the avgMovement size");

    auto& registry = controls::ControlRegistry::getInstance();
    const float rollingAvgAlpha = registry["state.rolling_avg_alpha"];
    const int sampleNum = registry["state.density_sample_count"];

    avgMovement->bindBuffer(0);
    data->bindBuffer(1);
    (*avgMovementCompute)["rollingAvgAlpha"] = rollingAvgAlpha;
    avgMovementCompute->dispatchCompute(avgMovement->getSize() / 64 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    sampleCount++;
    return sampleCount >= sampleNum;
}

void DensityControl::updatePositions(renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data)
{
    auto& registry = controls::ControlRegistry::getInstance();
    const float particleSpread = registry["state.particle_spread"];
    const float maxTargetParticleDensity = registry["state.max_target_particle_density"];
    const float particlePercantageToMove = registry["state.particle_percantage_to_move"];

    simManager->computeParticleDensities(data);
    avgMovement->mapBuffer(0, -1, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    std::sort(&(*avgMovement)[0], &(*avgMovement)[0] + avgMovement->getSize(),
              [](const AvgMovement& a, const AvgMovement& b) { return a.avgMovement < b.avgMovement; });
    avgMovement->unmapBuffer();

    avgMovement->bindBuffer(0);
    data->bindBuffer(1);
    (*updatePositionsCompute)["particleSpread"] = particleSpread;
    (*updatePositionsCompute)["maxTargetParticleDensity"] = maxTargetParticleDensity;
    (*updatePositionsCompute)["seedX"] = float(std::rand()) / RAND_MAX;
    (*updatePositionsCompute)["seedY"] = float(std::rand()) / RAND_MAX;
    updatePositionsCompute->dispatchCompute(GLuint(particlePercantageToMove / 100.0f * avgMovement->getSize()), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
