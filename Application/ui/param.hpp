#pragma once
#include <string>
#include <optional>
#include <glm/glm.hpp>
#include <initializer_list>
#include <vector>
#include <engine/renderEngine.h>
#include "../imgui/imgui.h"

class ImguiShowable
{
public:
	virtual void show(int screenWidth) = 0;
};

class Param : public ImguiShowable
{
public:
    Param(const std::string& name) : name(name) { }

protected:
    const std::string name;
};

class ParamFloat : public Param
{
public:
    ParamFloat(const std::string& name, const float value, const float min, const float max) 
        : Param(name), value(value), min(min), max(max) { }

    float value;

    void show(int screenWidth) override
    {
		ImGui::SetNextItemWidth(screenWidth / 5);
		ImGui::SliderFloat(name.c_str(), &value, min, max, "%.3f");
	}

private:
    const float min, max;
};

class ParamInt : public Param
{
public:
	ParamInt(const std::string& name, const int value, const int min, const int max) 
		: Param(name), value(value), min(min), max(max) { }

	int value;

    void show(int screenWidth) override
    {
		ImGui::SetNextItemWidth(screenWidth / 5);
		ImGui::SliderInt(name.c_str(), &value, min, max);
	}

private:
    const int min, max;
};

class ParamBool : public Param
{
public:
	ParamBool(const std::string& name, const bool value) 
		: Param(name), value(value) { }

	bool value;

	void show(int) override
	{
		ImGui::Checkbox(name.c_str(), &value);
	}
};

class ParamColor : public Param
{
public:
	ParamColor(const std::string& name, const glm::vec3& value) 
		: Param(name), value(value) { }

	glm::vec3 value;

	void show(int) override
	{
		ImGui::ColorEdit3(name.c_str(), (float*)&value, ImGuiColorEditFlags_NoInputs);
	}
};

class ParamLine : public ImguiShowable
{
public:
	ParamLine(std::initializer_list<Param*> params) : params(params), renderEnabledParam(nullptr) {}

	ParamLine(std::initializer_list<Param*> params, const ParamBool* renderEnabledParam) 
		: params(params), renderEnabledParam(renderEnabledParam) {}

	void show(int screenWidth) override
	{
		if(renderEnabledParam != nullptr && !renderEnabledParam->value)
			return;
		for (int i = 0; i < params.size(); i++)
		{
			params[i]->show(screenWidth);
			if (i < params.size() - 1)
				ImGui::SameLine();
		}
	}

private:
	const ParamBool* renderEnabledParam;
	std::vector<Param*> params;
};

class ParamLineCollection : public ImguiShowable
{
public:
	void addParamLines(std::initializer_list<ParamLine> paramLines)
	{
		for (auto& line : paramLines)
		{
			this->paramLines.push_back(line);
		}
	}

	void addParamLine(ParamLine line)
	{
		paramLines.push_back(line);
	}

	void show(int screenWidth) override
	{
		for (auto& line : paramLines)
		{
			line.show(screenWidth);
		}
	}

private:
	std::vector<ParamLine> paramLines;
};