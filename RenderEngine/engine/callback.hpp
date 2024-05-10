#pragma once

#include <functional>
#include <unordered_map>

template<typename... Args>
class Callback {
private:
	int counter = 0;
	std::unordered_map<int, std::function<void(Args...)>> callbacks;
	std::function<bool(Args...)> primaryCallback = {};

public:
	/**
	 * \brief Sets a callback function that will be called.
	 * \param callback - the callback function
	 * \return The id of the callback function
	 */
	int onCallback(std::function<void(Args...)>&& callback) {
		callbacks.insert(std::make_pair(counter, callback));
		return counter++;
	}

	/**
	 * \brief Removes a callback function with the given ID.
	 * \param id - the id of the callback function
	 */
	void removeCallbackFunction(int id) {
		callbacks.erase(id);
	}

	/**
	 * \brief Triggers the callback functions, including the primary callback (if there is any).
	 * \param args - the arguments to pass to the callback functions
	 */
	void triggerCallback(Args... args) {
		if (primaryCallback && !primaryCallback(args...))
		{
			return;
		}
		for (auto& cb : callbacks) {
			cb.second(args...);
		}
	}

	/**
	* \brief Sets the primary callback function that will be called, if it returns false, the other callbacks will not be called.
	* \param callback - the callback function
	*/
	void setPrimaryCallback(std::function<bool(Args...)>&& callback)
	{
		primaryCallback = callback;
	}

	/**
	 * \brief Removes the primary callback function.
	 */
	void removePrimaryCallback()
	{
		primaryCallback = {};
	}
};
