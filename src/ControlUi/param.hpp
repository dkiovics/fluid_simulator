#pragma once
#include <string>
#include <optional>
#include <glm/glm.hpp>
#include <initializer_list>
#include <vector>
#include <imgui.h>
#include <mutex>

namespace control
{

class ImguiShowable
{
public:
	virtual void show(int screenWidth) = 0;
};

class Param : public ImguiShowable
{
public:
	Param(const std::string& name) : name(name) {}

protected:
	std::mutex mutex;
	const std::string name;
};

class ParamFloat : public Param
{
public:
	ParamFloat(const std::string& name, const float value, const float min, const float max, const char* precision = "%.3f")
		: Param(name), value(value), min(min), max(max), precisionStr(precision)
	{
	}

	void show(int screenWidth) override
	{
		ImGui::SetNextItemWidth(screenWidth / 5);
		std::lock_guard<std::mutex> lock(mutex);
		ImGui::SliderFloat(name.c_str(), &value, min, max, precisionStr.c_str());
	}

	float getValue() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return value;
	}

	void setValue(float value)
	{
		std::lock_guard<std::mutex> lock(mutex);
		this->value = value;
	}

private:
	const float min, max;
	const std::string precisionStr;
	float value;
};

class ParamInt : public Param
{
public:
	ParamInt(const std::string& name, const int value, const int min, const int max)
		: Param(name), value(value), min(min), max(max)
	{
	}

	void show(int screenWidth) override
	{
		ImGui::SetNextItemWidth(screenWidth / 5);
		std::lock_guard<std::mutex> lock(mutex);
		ImGui::SliderInt(name.c_str(), &value, min, max);
	}

	int getValue() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return value;
	}

	void setValue(int value)
	{
		std::lock_guard<std::mutex> lock(mutex);
		this->value = value;
	}

private:
	const int min, max;
	int value;
};

class ParamBool : public Param
{
public:
	ParamBool(const std::string& name, const bool value)
		: Param(name), value(value)
	{
	}

	void show(int) override
	{
		std::lock_guard<std::mutex> lock(mutex);
		ImGui::Checkbox(name.c_str(), &value);
	}

	bool getValue() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return value;
	}

	void setValue(bool value)
	{
		std::lock_guard<std::mutex> lock(mutex);
		this->value = value;
	}

private:
	bool value;
};

class ParamButton : public Param
{
public:
	ParamButton(const std::string& name)
		: Param(name), wasPressed(false)
	{
	}

	void show(int) override
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (ImGui::Button(name.c_str()))
			wasPressed = true;
	}

	bool getValue() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		bool value = wasPressed;
		wasPressed = false;
		return value;
	}

private:
	bool wasPressed;
};

class ParamColor : public Param
{
public:
	ParamColor(const std::string& name, const glm::vec3& value)
		: Param(name), value(value)
	{
	}

	void show(int) override
	{
		std::lock_guard<std::mutex> lock(mutex);
		ImGui::ColorEdit3(name.c_str(), (float*)&value, ImGuiColorEditFlags_NoInputs);
	}

	glm::vec3 getValue() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return value;
	}

	void setValue(const glm::vec3& value)
	{
		std::lock_guard<std::mutex> lock(mutex);
		this->value = value;
	}

private:
	glm::vec3 value;
};

class ParamRadio : public Param
{
public:
	ParamRadio(const std::string& name, const std::vector<std::string>& options, const int value)
		: Param(name), options(options), value(value)
	{
	}

	const std::vector<std::string> options;

	void show(int) override
	{
		std::lock_guard<std::mutex> lock(mutex);
		for (int i = 0; i < options.size(); i++)
		{
			if (ImGui::RadioButton(options[i].c_str(), value == i))
				value = i;
			if (i < options.size() - 1)
				ImGui::SameLine();
		}
	}

	int getValue() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return value;
	}

	void setValue(int value)
	{
		std::lock_guard<std::mutex> lock(mutex);
		this->value = value;
	}

private:
	int value;
};

class ParamText : public Param
{
public:
	ParamText(const std::string& name, const std::string& value, size_t bufferSize = 150)
		: Param(name), value(new char[bufferSize]), bufferSize(bufferSize)
	{
		strcpy(this->value, value.c_str());
	}

	ParamText(ParamText&& other) noexcept
		: Param(other.name), value(other.value), bufferSize(other.bufferSize)
	{
		other.value = nullptr;
	}

	ParamText& operator=(ParamText&& other) noexcept
	{
		if (this != &other)
		{
			delete[] value;
			name = other.name;
			value = other.value;
			bufferSize = other.bufferSize;
			other.value = nullptr;
		}
		return *this;
	}

	ParamText(const ParamText&) = delete;
	ParamText& operator=(const ParamText&) = delete;

	std::string getValue() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return value;
	}

	void setValue(const std::string& value)
	{
		std::lock_guard<std::mutex> lock(mutex);
		strcpy(this->value, value.c_str());
	}

	void show(int) override
	{
		ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.3f);
		std::lock_guard<std::mutex> lock(mutex);
		ImGui::InputText(name.c_str(), value, bufferSize);
	}

	~ParamText()
	{
		delete[] value;
	}

private:
	char* value;
	const size_t bufferSize;

};

class ParamLine : public ImguiShowable
{
public:
	ParamLine(std::initializer_list<Param*> params) : params(params), renderEnabledParam(nullptr) {}

	ParamLine(std::initializer_list<Param*> params, const ParamBool* renderEnabledParam)
		: params(params), renderEnabledParam(renderEnabledParam)
	{
	}

	void show(int screenWidth) override
	{
		if (renderEnabledParam != nullptr && !renderEnabledParam->value)
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

} // namespace control
