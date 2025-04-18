#pragma once
#include <memory>
#include <map>
#include <set>
#include <string>
#include <mutex>
#include <variant>
#include <glm/glm.hpp>
#include <stdexcept>
#include <sstream>

namespace controls
{

class _ControlRegistryPrivate;

class ControlRegistry
{
public:
	typedef std::variant<float, int, bool, std::string, glm::vec3> NodeValue;

private:
	static std::shared_ptr<ControlRegistry> instance;

	ControlRegistry() = default;
	ControlRegistry(const ControlRegistry&) = delete;
	ControlRegistry& operator=(const ControlRegistry&) = delete;
	ControlRegistry(ControlRegistry&&) = delete;
	ControlRegistry& operator=(ControlRegistry&&) = delete;

	typedef std::map<std::string, NodeValue> ControlMap;

	ControlMap controlValues;
	std::mutex controlValuesMutex;

	std::set<std::string> modifiedValues;

	friend _ControlRegistryPrivate;

public:
	static ControlRegistry& getInstance();

	class NodeProxy
	{
	private:
		std::string nodePath;
		ControlMap& controlValues;
		std::mutex& mutex;

	public:
		NodeProxy(const std::string& path, ControlMap& controlValues, std::mutex& mutex)
			: nodePath(path), controlValues(controlValues), mutex(mutex)
		{ }

		template<typename T>
		const T& operator=(const T& value)
		{
			std::lock_guard<std::mutex> lock(mutex);
			auto it = controlValues.find(nodePath);
			if (it == controlValues.end())
			{
				throw std::runtime_error("Control node not found: " + nodePath);
			}
			if (!std::holds_alternative<T>(it->second))
			{
				throw std::runtime_error("Type mismatch for control node: " + nodePath);
			}
			return std::get<T>(it->second) = value;
		}

		template<typename T>
		operator T() const
		{
			std::lock_guard<std::mutex> lock(mutex);
			auto it = controlValues.find(nodePath);
			if (it == controlValues.end())
			{
				throw std::runtime_error("Control node not found: " + nodePath);
			}
			if (!std::holds_alternative<T>(it->second))
			{
				throw std::runtime_error("Type mismatch for control node: " + nodePath);
			}
			return std::get<T>(it->second);
		}
	};

	NodeProxy operator[](const std::string& path)
	{
		return NodeProxy(path, controlValues, controlValuesMutex);
	}
};

} // namespace controls
