#include "prefixSum.h"

using namespace renderer;

PrefixSum::PrefixSum()
{
    localScan = make_compute("shaders/utils/prefixSum/localScan.comp");
    sumScan = make_compute("shaders/utils/prefixSum/sumScan.comp");
    offsetScan = make_compute("shaders/utils/prefixSum/offsetScan.comp");
}

void PrefixSum::runInclusive(ssbo_ptr<uint32_t> data)
{
    const unsigned int dataSize = data->getSize();
    if (dataSize <= 1)
        return;

    std::vector<unsigned int> wgCounts;
    unsigned int items = dataSize;
    while (items > 1)
    {
        const unsigned int wg = (items + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
        wgCounts.push_back(wg);
        items = wg;
    }

    while (partialSumTower.size() + 1 < wgCounts.size())
    {
        partialSumTower.push_back(make_ssbo<uint32_t>(1, GL_DYNAMIC_COPY));
    }
    while (partialSumTower.size() + 1 > wgCounts.size())
    {
        partialSumTower.pop_back();
    }
    for (size_t i = 0; i + 1 < wgCounts.size(); i++)
    {
        partialSumTower[i]->setSize(wgCounts[i]);
    }

    auto bindLevel = [&](GLuint bindingId, size_t level) {
        if (level == 0)
            data->bindBuffer(bindingId);
        else
            partialSumTower[level - 1]->bindBuffer(bindingId);
    };

    for (size_t i = 0; i + 1 < wgCounts.size(); i++)
    {
        bindLevel(0, i);
        bindLevel(1, i + 1);
        localScan->dispatchCompute(wgCounts[i], 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    bindLevel(1, wgCounts.size() - 1);
    sumScan->dispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    for (int i = (int) wgCounts.size() - 2; i >= 0; i--)
    {
        bindLevel(0, i);
        bindLevel(1, i + 1);
        offsetScan->dispatchCompute(wgCounts[i], 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
}
