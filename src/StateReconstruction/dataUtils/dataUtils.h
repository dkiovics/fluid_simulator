#pragma once

#include <compute/computeProgram.h>
#include <compute/storageBuffer.h>
#include "manager/simulationManager.h"

namespace diffrender
{

class MiscDataUtils
{
public:
    MiscDataUtils();

    MiscDataUtils(const MiscDataUtils& other) = delete;
    MiscDataUtils(MiscDataUtils&& other) noexcept = delete;
    MiscDataUtils& operator=(const MiscDataUtils& other) = delete;
    MiscDataUtils& operator=(MiscDataUtils&& other) noexcept = delete;

    void clampParticlePositions(renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
                                glm::vec3 lowerBound,
                                glm::vec3 upperBound);

private:
    renderer::compute_ptr clampPositionsCompute;
};

class DensityControl
{
public:
    DensityControl(std::shared_ptr<genericfsim::manager::SimulationManager> simManager);

    DensityControl(const DensityControl& other) = delete;
    DensityControl(DensityControl&& other) noexcept = delete;
    DensityControl& operator=(const DensityControl& other) = delete;
    DensityControl& operator=(DensityControl&& other) noexcept = delete;

    void init(size_t particleNum);

    bool updateAvgMovement(renderer::ssbo_ptr<float> data);

    void updatePositions(renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data);

private:
    struct AvgMovement
    {
        float avgMovement = 0.0f;
        int index = 0;
    };

    int sampleCount = 0;

    renderer::compute_ptr avgMovementCompute;
    renderer::compute_ptr initAvgMovementArray;
    renderer::compute_ptr updatePositionsCompute;

    renderer::ssbo_ptr<AvgMovement> avgMovement;

    std::shared_ptr<genericfsim::manager::SimulationManager> simManager;
};

} // namespace diffrender
