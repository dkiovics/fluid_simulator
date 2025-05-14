#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "engine/windowManager.h"


namespace controls
{

class ControlWindow
{
private:
	std::unique_ptr<std::thread> renderThread;

	std::atomic_bool running = false;
	std::atomic_bool startupFinished = false;

	std::mutex mutex;
	std::condition_variable startSignal;

	void renderLoop();

	const int initialWidth;
	const int initialHeight;

public:
	ControlWindow(int initialWith, int initialHeight);

	ControlWindow(const ControlWindow&) = delete;
	ControlWindow& operator=(const ControlWindow&) = delete;
	ControlWindow(ControlWindow&&) = delete;
	ControlWindow& operator=(ControlWindow&&) = delete;

	~ControlWindow();

	void start();

	void stop() noexcept;

	bool isRunning() const noexcept;
};

}
