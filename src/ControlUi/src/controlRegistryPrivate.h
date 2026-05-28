#pragma once

#include "controlRegistry.h"
#include <functional>

namespace controls
{

class _ControlRegistryPrivate
{
private:
	static std::unique_ptr<_ControlRegistryPrivate> instance;

	std::shared_ptr<ControlRegistry> controlRegistry;

	std::string lastLoadedPath;

	_ControlRegistryPrivate();
	_ControlRegistryPrivate(const _ControlRegistryPrivate&) = delete;
	_ControlRegistryPrivate& operator=(const _ControlRegistryPrivate&) = delete;
	_ControlRegistryPrivate(_ControlRegistryPrivate&&) = delete;
	_ControlRegistryPrivate& operator=(_ControlRegistryPrivate&&) = delete;

	ControlRegistry::ControlMap uiThreadControlMap;

public:
	static _ControlRegistryPrivate& getInstance();

	template<typename T>
	T& get(const std::string& path)
	{
		auto it = uiThreadControlMap.find(path);
		if (it == uiThreadControlMap.end())
		{
			throw std::runtime_error("Control not found");
		}
		return std::get<T>(it->second);
	}

	template<typename T>
	T getOrDefault(const std::string& path, const T& defaultValue)
	{
		auto it = uiThreadControlMap.find(path);
		if (it == uiThreadControlMap.end())
		{
			return defaultValue;
		}
		return std::get<T>(it->second);
	}

	bool isControlRegistered(const std::string& path) const;

	void registerControl(const std::string& path, const ControlRegistry::NodeValue& value);

	bool saveControlValues(const std::string& path);

	bool loadControlValues(const std::string& path);

	void syncControlValues();

};

} // namespace control
