#pragma once
#include <glm/glm.hpp>
#include <imgui.h>
#include <variant>
#include <vector>
#include "controlRegistryPrivate.h"

namespace controls
{

class IControlElement
{
protected:
	const std::string path;
	const std::string text;
	_ControlRegistryPrivate& registry;

public:
	IControlElement(const std::string& path, const std::string& text, const ControlRegistry::NodeValue& init)
		: path(path), text(text), registry(_ControlRegistryPrivate::getInstance())
	{
		if (!registry.isControlRegistered(path))
			registry.registerControl(path, init);
	}

	virtual void draw() = 0;
};

class Button : public IControlElement
{
public:
	Button(const std::string& path, const std::string& text)
		: IControlElement(path, text, false)
	{ }

	void draw() override
	{
		if (ImGui::Button(text.c_str()))
		{
			registry.get<bool>(path) = true;
		}
	}
};

class CheckBox : public IControlElement
{
public:
	CheckBox(const std::string& path, const std::string& text, bool init = false)
		: IControlElement(path, text, init)
	{ }

	void draw() override
	{
		ImGui::Checkbox(text.c_str(), &registry.get<bool>(path));
	}
};

class SliderFloat : public IControlElement
{
private:
	const float min;
	const float max;
	const std::string format;

public:
	SliderFloat(const std::string& path, const std::string& text, float min, float max, float defaultVal, const std::string& format = "")
		: IControlElement(path, text, defaultVal), min(min), max(max), format(format)
	{ }

	void draw() override
	{
		if (format.empty())
			ImGui::SliderFloat(text.c_str(), &registry.get<float>(path), min, max);
		else
			ImGui::SliderFloat(text.c_str(), &registry.get<float>(path), min, max, format.c_str());
	}
};

class SliderInt : public IControlElement
{
private:
	const int min;
	const int max;

public:
	SliderInt(const std::string& path, const std::string& text, int min, int max, int defaultVal)
		: IControlElement(path, text, defaultVal), min(min), max(max)
	{ }

	void draw() override
	{
		ImGui::SliderInt(text.c_str(), &registry.get<int>(path), min, max);
	}
};

class ColorPicker : public IControlElement
{
public:
	ColorPicker(const std::string& path, const std::string& text, glm::vec3 color = glm::vec3(1.0f))
		: IControlElement(path, text, color)
	{ }

	void draw() override
	{
		ImGui::ColorEdit3(text.c_str(), &registry.get<glm::vec3>(path)[0], ImGuiColorEditFlags_NoInputs);
	}
};

class RadioButton : public IControlElement
{
public:
	std::vector<std::string> options;

	RadioButton(const std::string& path, const std::vector<std::string>& options, int init = 0)
		: IControlElement(path, "", init), options(options)
	{ }

	void draw() override
	{
		for (size_t i = 0; i < options.size(); ++i)
		{
			ImGui::RadioButton(options[i].c_str(), &registry.get<int>(path), i);
			if (i < options.size() - 1)
				ImGui::SameLine();
		}
	}
};

class TextInput : public IControlElement
{
public:
	TextInput(const std::string& path, const std::string& text, const std::string& init = "")
		: IControlElement(path, text, init)
	{ }

	void draw() override
	{
		char buffer[256];
		strncpy(buffer, registry.get<std::string>(path).c_str(), sizeof(buffer));
		if (ImGui::InputText(text.c_str(), buffer, sizeof(buffer)))
		{
			registry.get<std::string>(path) = buffer;
		}
	}
};

} // namespace controls
