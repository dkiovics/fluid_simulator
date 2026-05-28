#pragma once

#include "computeProgram.h"
#include "storageBuffer.h"

namespace renderer
{

class PrefixSum
{
public:
    PrefixSum();

    /**
     * \brief Runs an in-place inclusive prefix sum over the contents of `data`.
     * After the call, data[i] == sum_{k=0..i} original_data[k] and a
     * GL_SHADER_STORAGE_BARRIER_BIT memory barrier has been issued.
     */
    void runInclusive(ssbo_ptr<uint32_t> data);

private:
    static constexpr unsigned int WORK_GROUP_SIZE = 1024;

    compute_ptr localScan;
    compute_ptr sumScan;
    compute_ptr offsetScan;

    // Levels above `data`: partialSumTower[i] holds sums for level i+1.
    std::vector<ssbo_ptr<uint32_t>> partialSumTower;
};

} // namespace renderer
