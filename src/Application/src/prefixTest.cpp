#include "prefixTest.h"

#include <vector>
#include "engine/windowManager.h"
#include "compute/storageBuffer.h"
#include "compute/computeProgram.h"
#include "spdlog/spdlog.h"

static constexpr int TEST_LIST_SIZE = 1000000;
static constexpr int WORK_GROUP_SIZE = 1024;

void runPrefixTest()
{
	const int workGroupCount = (TEST_LIST_SIZE + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;

	spdlog::info("************************************************");
	spdlog::info("Running prefix test with {} elements and {} work groups", TEST_LIST_SIZE, workGroupCount);

	spdlog::info("Initializing OpenGL context...");
	renderer::WindowManager dummyWindow(500, 1, "Dummy");
	
	renderer::ssbo_ptr<uint32_t> valueList = renderer::make_ssbo<uint32_t>(TEST_LIST_SIZE, GL_DYNAMIC_COPY);
	renderer::ssbo_ptr<uint32_t> partialSums = renderer::make_ssbo<uint32_t>(workGroupCount, GL_DYNAMIC_COPY);
	std::vector<uint32_t> valueListData(TEST_LIST_SIZE);

	renderer::ComputeProgram localScanProgram("shaders/prefixTest/localScan.comp");
	renderer::ComputeProgram sumScanProgram("shaders/prefixTest/sumScan.comp");
	renderer::ComputeProgram offsetScanProgram("shaders/prefixTest/offsetScan.comp");

	spdlog::info("Compute programs loaded, generating data...");

	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	for (auto& i : valueListData)
	{
		i = std::rand() % 100;
	}
	valueList->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	valueList->setData(valueListData.data(), 0, TEST_LIST_SIZE);
	valueList->unmapBuffer();

	valueList->bindBuffer(0);
	partialSums->bindBuffer(1);

	spdlog::info("Data generated, running local scan...");
	localScanProgram.dispatchCompute(workGroupCount, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	spdlog::info("Running sum scan...");
	sumScanProgram.dispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	spdlog::info("Running offset scan...");
	offsetScanProgram.dispatchCompute(workGroupCount, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	spdlog::info("Checking results...");

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
