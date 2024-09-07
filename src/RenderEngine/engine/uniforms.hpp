#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <vector>
#include <iostream>
#include <initializer_list>
#include "shaderProgram.h"
#include "texture.h"

namespace renderer
{

class UniformProvider
{
public:
	/**
	* \brief Sets the uniforms of the given ShaderProgram.
	* \param program - The ShaderProgram to set the uniforms of.
	* \param prefix - The prefix to use for the uniform names.
	*/
	virtual void setUniforms(const ShaderProgram& program, const std::string& prefix = "") const = 0;
};

class UniformGatherer : public UniformProvider
{
public:
	void setUniforms(const ShaderProgram& program, const std::string& prefix = "") const override
	{
		for (const UniformProvider* provider : providers)
		{
			provider->setUniforms(program, ignorePrefix ? objectName : (prefix + objectName));
		}
	}

	UniformGatherer(const UniformGatherer&) = delete;
	UniformGatherer& operator=(const UniformGatherer&) = delete;
	UniformGatherer(UniformGatherer&& other) = delete;
	UniformGatherer& operator=(UniformGatherer&& other) = delete;

protected:
	template<typename U1, typename... Un>
	UniformGatherer(const std::string& objectName, bool ignorePrefix, const U1& u1, const Un&... args) 
		: objectName(objectName), ignorePrefix(ignorePrefix)
	{
		append(u1, args...);
	}

	template<typename U1, typename... Un>
	void addUniform(const U1& u1, const Un&... args)
	{
		append(u1, args...);
	}

private:
	bool ignorePrefix;
	std::vector<const UniformProvider*> providers;
	std::string objectName;

	template<typename U1, typename... Un>
	void append(const U1& u1, const Un&... args)
	{
		auto provider = dynamic_cast<const UniformProvider*>(&u1);
		providers.push_back(provider);
		append(args...);
	}

	template<typename U1>
	void append(const U1& u1)
	{
		auto provider = dynamic_cast<const UniformProvider*>(&u1);
		providers.push_back(provider);
	}
};

class UniformGathererGlobal
{
public:
	/**
	* \brief Adds the given ShaderPrograms to the list of programs to set the uniforms of whenever the uniforms change.
	* \param args - The ShaderPrograms to add.
	*/
	void addProgram(std::initializer_list<std::shared_ptr<ShaderProgram>> args)
	{
		for (auto& p : args)
		{
			programs.push_back(p);
		}
	}

	/**
	 * \brief Sets the uniforms of all ShaderPrograms in the list of programs.
	 */
	void setUniformsForAllPrograms() const
	{
		for (const auto& program : programs)
		{
			setUniformsGlobal(*program);
		}
	}

protected:
	/**
	* \brief Sets the uniforms of the given ShaderProgram.
	* \param program - The ShaderProgram to set the uniforms of.
	*/
	virtual void setUniformsGlobal(const ShaderProgram& program) const = 0;

private:
	std::vector<std::shared_ptr<ShaderProgram>> programs;
};

template<typename V>
class UniformVariable : public UniformProvider
{
public:
	/**
	 * \brief Creates a new UniformVariable with the given name.
	 * \param name - The name of the uniform variable.
	 */
	UniformVariable(const std::string& name) : name(name) { }

	void setUniforms(const ShaderProgram& program, const std::string& prefix = "") const override
	{
		if (value.has_value())
		{
			program[prefix + name] = value.value();
		}
	}

	/**
	 * \brief Sets the value of the uniform variable.
	 * \param value - The value to set the uniform variable to.
	 * \return The value of the uniform variable.
	 */
	V& operator=(const V& value)
	{
		this->value = value;
		return this->value.value();
	}

	/**
	 * \brief Sets the value of the uniform variable.
	 * \param value - The value to set the uniform variable to.
	 * \return The value of the uniform variable.
	 */
	V& operator=(V&& value)
	{
		this->value = std::move(value);
		return this->value.value();
	}

	V& operator*()
	{
		return value.value();
	}

	const V& operator*() const
	{
		return value.value();
	}

private:
	const std::string name;
	std::optional<V> value;

};

template<>
class UniformVariable<Texture> : public UniformProvider
{
public:
	/**
	 * \brief Creates a new UniformVariable with the given name.
	 * \param name - The name of the uniform variable.
	 */
	UniformVariable(const std::string& name) : name(name) {}

	UniformVariable(const UniformVariable&) = delete;
	UniformVariable& operator=(const UniformVariable&) = delete;

	void setUniforms(const ShaderProgram& program, const std::string& prefix) const override
	{
		if (value)
		{
			program[prefix + name] = *value;
		}
	}

	/**
	 * \brief Sets the value of the uniform variable.
	 * \param value - The value to set the uniform variable to.
	 * \return The value of the uniform variable.
	 */
	std::shared_ptr<Texture>& operator=(std::shared_ptr<Texture> texture)
	{
		this->value = texture;
		return this->value;
	}

	Texture& operator*()
	{
		return *value;
	}

	const Texture& operator*() const
	{
		return *value;
	}

private:
	const std::string name;
	std::shared_ptr<Texture> value;
};

template<typename V>
class UniformPointer : public UniformProvider
{
public:
	UniformPointer() { }

	UniformPointer(const UniformPointer&) = delete;
	UniformPointer& operator=(const UniformPointer&) = delete;

	void setUniforms(const ShaderProgram& program, const std::string& prefix) const override
	{
		if (value)
		{
			value->setUniforms(program, prefix);
		}
	}

	/**
	 * \brief Sets the value of the uniform pointer.
	 * \param value - The value to set the uniform pointer to.
	 * \return The value of the uniform pointer.
	 */
	std::shared_ptr<V>& operator=(std::shared_ptr<V> value)
	{
		this->value = value;
		return this->value;
	}

	V& operator*()
	{
		return *value;
	}

	const V& operator*() const
	{
		return *value;
	}

private:
	std::shared_ptr<V> value;
};


#define u_var(name, type) \
	UniformVariable<type> name = UniformVariable<type>(#name)

#define u_ptr(name, type) \
	UniformPointer<type> name = UniformPointer<type>()

} // namespace renderer
