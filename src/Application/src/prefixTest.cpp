#include "prefixTest.h"

#include <vector>
#include "engine/windowManager.h"
#include "compute/storageBuffer.h"
#include "compute/computeProgram.h"
#include "spdlog/spdlog.h"

static constexpr int TEST_LIST_SIZE = 800000000;
static constexpr int WORK_GROUP_SIZE = 1024;

void runPrefixTest()
{
	spdlog::info("************************************************");
	spdlog::info("Running prefix test with {} elements", TEST_LIST_SIZE);

	spdlog::info("Initializing OpenGL context...");
	renderer::WindowManager dummyWindow(500, 1, "Dummy");

	spdlog::info("Creating work group counts...");
	std::vector<int> workGroupCounts;
	int itemsInLastGroup = TEST_LIST_SIZE;
	while (itemsInLastGroup > 1)
	{
		int workGroupCount = (itemsInLastGroup + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
		workGroupCounts.push_back(workGroupCount);
		itemsInLastGroup = workGroupCount;
	}
	spdlog::info("Sum tree created with {} levels", workGroupCounts.size());

	spdlog::info("Loading compute programs...");
	renderer::ComputeProgram localScanProgram("shaders/prefixTest/localScan.comp");
	renderer::ComputeProgram sumScanProgram("shaders/prefixTest/sumScan.comp");
	renderer::ComputeProgram offsetScanProgram("shaders/prefixTest/offsetScan.comp");

	spdlog::info("Creating work group SSBOs...");
	std::vector<renderer::ssbo_ptr<uint32_t>> workGroupSSBOs;
	workGroupSSBOs.push_back(renderer::make_ssbo<uint32_t>(TEST_LIST_SIZE, GL_DYNAMIC_COPY));
	for (int i = 0; i < workGroupCounts.size() - 1; ++i)
	{
		int workGroupCount = workGroupCounts[i];
		workGroupSSBOs.push_back(renderer::make_ssbo<uint32_t>(workGroupCount, GL_DYNAMIC_COPY));
	}

	spdlog::info("Compute programs loaded, generating data...");
	std::vector<uint32_t> valueListData(TEST_LIST_SIZE);
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	for (auto& i : valueListData)
	{
		i = std::rand() % 100;
	}
	workGroupSSBOs[0]->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	workGroupSSBOs[0]->setData(valueListData.data(), 0, TEST_LIST_SIZE);
	workGroupSSBOs[0]->unmapBuffer();
	spdlog::info("Data generated and uploaded to GPU");

	spdlog::info("Running up-sweep...");
	for (int i = 0; i < workGroupCounts.size() - 1; ++i)
	{
		int workGroupCount = workGroupCounts[i];
		workGroupSSBOs[i]->bindBuffer(0);
		workGroupSSBOs[i + 1]->bindBuffer(1);
		localScanProgram.dispatchCompute(workGroupCount, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
	
	spdlog::info("Running prefix scan on topmost work group...");
	workGroupSSBOs.back()->bindBuffer(1);
	sumScanProgram.dispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	spdlog::info("Running down-sweep...");
	for (int i = workGroupCounts.size() - 2; i >= 0; --i)
	{
		int workGroupCount = workGroupCounts[i];
		workGroupSSBOs[i]->bindBuffer(0);
		workGroupSSBOs[i + 1]->bindBuffer(1);
		offsetScanProgram.dispatchCompute(workGroupCount, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	spdlog::info("Waiting for GPU to finish...");
	auto fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
	glDeleteSync(fence);
	spdlog::info("GPU finished");

	spdlog::info("Checking results...");
	auto& valueList = workGroupSSBOs[0];
	valueList->mapBuffer(0, -1, GL_MAP_READ_BIT);
	bool correct = true;
	for (int i = 0; i < TEST_LIST_SIZE; ++i)
	{
		if (i > 0)
		{
			valueListData[i] += valueListData[i - 1];
		}
		if (valueListData[i] != (*valueList)[i])
		{
			spdlog::error("Error at index {}: expected {}, got {}", i, valueListData[i], (*valueList)[i]);
			correct = false;
		}
	}
	valueList->unmapBuffer();
	if (correct)
		spdlog::info("Results are correct!");
	
	spdlog::info("************************************************");
}
