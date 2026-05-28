#include "prefixTest.h"

#include <vector>
#include "compute/prefixSum.h"
#include "compute/storageBuffer.h"
#include "engine/windowManager.h"
#include "spdlog/spdlog.h"

static constexpr int TEST_LIST_SIZE = 800000000;

void runPrefixTest()
{
	spdlog::info("************************************************");
	spdlog::info("Running prefix test with {} elements", TEST_LIST_SIZE);

	spdlog::info("Initializing OpenGL context...");
	renderer::WindowManager dummyWindow(500, 1, "Dummy");

	spdlog::info("Loading prefix sum helper...");
	renderer::PrefixSum prefixSum;

	spdlog::info("Allocating data buffer and generating data...");
	auto dataBuffer = renderer::make_ssbo<uint32_t>(TEST_LIST_SIZE, GL_DYNAMIC_COPY);
	std::vector<uint32_t> valueListData(TEST_LIST_SIZE);
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	for (auto& i : valueListData)
	{
		i = std::rand() % 100;
	}
	dataBuffer->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	dataBuffer->setData(valueListData.data(), 0, TEST_LIST_SIZE);
	dataBuffer->unmapBuffer();
	spdlog::info("Data generated and uploaded to GPU");

	spdlog::info("Running inclusive prefix sum...");
	prefixSum.runInclusive(dataBuffer);

	spdlog::info("Waiting for GPU to finish...");
	auto fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glClientWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
	glDeleteSync(fence);
	spdlog::info("GPU finished");

	spdlog::info("Checking results...");
	dataBuffer->mapBuffer(0, -1, GL_MAP_READ_BIT);
	bool correct = true;
	for (int i = 0; i < TEST_LIST_SIZE; ++i)
	{
		if (i > 0)
		{
			valueListData[i] += valueListData[i - 1];
		}
		if (valueListData[i] != (*dataBuffer)[i])
		{
			spdlog::error("Error at index {}: expected {}, got {}", i, valueListData[i], (*dataBuffer)[i]);
			correct = false;
		}
	}
	dataBuffer->unmapBuffer();
	if (correct)
		spdlog::info("Results are correct!");

	spdlog::info("************************************************");
}
