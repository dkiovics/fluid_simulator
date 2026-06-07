#include "controlRegistryPrivate.h"
#include <fstream>
#include <spdlog/spdlog.h>

using namespace controls;

_ControlRegistryPrivate::_ControlRegistryPrivate()
{
	ControlRegistry::getInstance();
	controlRegistry = ControlRegistry::instance;
}

std::unique_ptr<_ControlRegistryPrivate> _ControlRegistryPrivate::instance;

_ControlRegistryPrivate& _ControlRegistryPrivate::getInstance()
{
	if (!instance)
	{
		instance = std::unique_ptr<_ControlRegistryPrivate>(new _ControlRegistryPrivate());
	}
	return *instance;
}

bool _ControlRegistryPrivate::isControlRegistered(const std::string& path) const
{
	return uiThreadControlMap.find(path) != uiThreadControlMap.end();
}

void _ControlRegistryPrivate::registerControl(const std::string& path, const ControlRegistry::NodeValue& value)
{
	if (isControlRegistered(path))
	{
		spdlog::warn("ControlRegistry: Control already registered at path: {}", path);
	}
	std::lock_guard<std::mutex> lock(controlRegistry->controlValuesMutex);
	controlRegistry->controlValues[path] = value;
	uiThreadControlMap[path] = value;
}

bool _ControlRegistryPrivate::saveControlValues(const std::string& path)
{
	std::ofstream saveFile(path);
	if (!saveFile.is_open())
	{
		return false;
	}

	syncControlValues();

	std::lock_guard<std::mutex> lock(controlRegistry->controlValuesMutex);

	for (const auto& it : controlRegistry->controlValues)
	{
		const auto& path = it.first;
		const auto& value = it.second;
		if (std::holds_alternative<bool>(value))
			saveFile << path << " b = " << std::get<bool>(value) << "\n";
		else if (std::holds_alternative<int>(value))
			saveFile << path << " i = " << std::get<int>(value) << "\n";
		else if (std::holds_alternative<float>(value))
			saveFile << path << " f = " << std::get<float>(value) << "\n";
		else if (std::holds_alternative<std::string>(value))
			saveFile << path << " s = \"" << std::get<std::string>(value) << "\"\n";
		else if (std::holds_alternative<glm::vec3>(value))
			saveFile << path << " v3 = " << std::get<glm::vec3>(value).x << " " << std::get<glm::vec3>(value).y << " " << std::get<glm::vec3>(value).z << "\n";
		else
			spdlog::warn("ControlRegistry: Unknown type for control value at path: {}", path);
	};

	saveFile.close();
}

bool _ControlRegistryPrivate::loadControlValues(const std::string& path)
{
	std::ifstream loadFile(path);
	if (!loadFile.is_open())
	{
		return false;
	}

	std::string line;
	while (std::getline(loadFile, line))
	{
		std::istringstream iss(line);
		std::string path, type, _;
		if (!(iss >> path >> type >> _))
		{
			spdlog::warn("ControlRegistry: Failed to parse line: {}", line);
			continue;
		}

		if (type == "b")
		{
			bool value;
			if (iss >> value)
				registerControl(path, value);
		}
		else if (type == "i")
		{
			int value;
			if (iss >> value)
				registerControl(path, value);
		}
		else if (type == "f")
		{
			float value;
			if (iss >> value)
				registerControl(path, value);
		}
		else if (type == "s")
		{
			std::string value;
			if (iss >> std::quoted(value))
				registerControl(path, value);
		}
		else if (type == "v3")
		{
			glm::vec3 value;
			if (iss >> value.x >> value.y >> value.z)
				registerControl(path, value);
		}
		else
			spdlog::warn("ControlRegistry: Unknown type for control value at path: {}", path);
	}

	loadFile.close();
}

void _ControlRegistryPrivate::syncControlValues()
{
	std::lock_guard<std::mutex> lock(controlRegistry->controlValuesMutex);
	for (const auto& s : controlRegistry->modifiedValues)
	{
		uiThreadControlMap[s] = controlRegistry->controlValues[s];
	}
	controlRegistry->modifiedValues.clear();
	controlRegistry->controlValues = uiThreadControlMap;
}
