#pragma once

#include <functional>
#include <unordered_map>

template<typename... Args>
class Callback {
private:
	int counter = 0;
	std::unordered_map<int, std::function<void(Args...)>> callbacks;

public:
	int onCallback(std::function<void(Args...)>&& callback) {
		callbacks.insert(std::make_pair(counter, callback));
		return counter++;
	}

	void removeCallbackFunction(int id) {
		callbacks.erase(id);
	}

	void triggerCallback(Args... args) {
		for (auto& cb : callbacks) {
			cb.second(args...);
		}
	}
};
