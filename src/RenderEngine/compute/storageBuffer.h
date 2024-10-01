#pragma once

#include "../engine/renderEngine.h"
#include <mutex>
#include <map>
#include <set>

namespace renderer
{

template <typename T>
class StorageBuffer
{
public:
	/**
	 * \brief Creates a new storage buffer with the given size and usage.
	 * 
	 * \param size - The size of the buffer in sizeof(T) units. T must not contain mat3 or vec3 types.
	 * \param usage - The usage of the buffer. Must be GL_<1>_<2>, where <1> is STATIC, DYNAMIC or STREAM and <2> is DRAW, READ or COPY.
	 * 
	 *	STATIC means that the data will be set once and used many times.
	 *	DYNAMIC means that the data will be set many times and used many times.
	 *	STREAM means that the data will be set once and used at most a few times.
	 *
	 *	DRAW means that the data will be set by the CPU and read by the GPU.
	 *	READ means that the data will be set by the GPU and read by the CPU.
	 *	COPY means that the data will be set by the GPU and read by the GPU.
	 */
	StorageBuffer(unsigned int size, GLenum usage) : usage(usage), size(size), renderEngine(RenderEngine::getInstance())
	{
		glGenBuffers(1, &bufferId);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(T), NULL, usage);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	StorageBuffer(const StorageBuffer<T>&) = delete;
	StorageBuffer(StorageBuffer<T>&&) = delete;
	StorageBuffer<T>& operator=(const StorageBuffer<T>&) = delete;
	StorageBuffer<T>& operator=(StorageBuffer<T>&&) = delete;

	~StorageBuffer()
	{
		unmapBuffer();
		glDeleteBuffers(1, &bufferId);
	}

	/**
	 * \brief Sets the buffer size, resizing it if necessary. The buffer must be unmapped before calling this function.
	 * 
	 * \param size - The new size of the buffer in sizeof(T) units.
	 */
	void setSize(unsigned int size)
	{
		if (mappedData != nullptr)
		{
			throw std::runtime_error("Buffer must be unmapped before resizing.");
		}

		if (size == this->size)
		{
			return;
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferId);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(T), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		this->size = size;
	}

	unsigned int getSize() const
	{
		return size;
	}

	void fillWithZeros() const
	{
		if (mappedData != nullptr)
		{
			throw std::runtime_error("Buffer must be unmapped before filling it with zeros.");
		}

		glClearNamedBufferData(bufferId, GL_R8UI, GL_RED, GL_UNSIGNED_BYTE, NULL);
	}

	/**
	 * \brief Maps the buffer to the CPU with the given access.
	 * 
	 * \param start - The start index of the buffer to map.
	 * \param size - The size of the buffer to map in sizeof(T) units. If -1, maps the whole buffer starting from start.
	 * \param access - The access to the buffer. Must be a combination of GL_MAP_READ_BIT, GL_MAP_WRITE_BIT, 
	 * GL_MAP_PERSISTENT_BIT and GL_MAP_COHERENT_BIT. Optional flags are GL_MAP_INVALIDATE_RANGE_BIT,
	 * GL_MAP_INVALIDATE_BUFFER_BIT.
	 */
	void mapBuffer(unsigned int start = 0, unsigned int size = -1, GLbitfield access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)
	{
		if (mappedData != nullptr)
		{
			throw std::runtime_error("Buffer is already mapped.");
		}

		if (size == -1)
		{
			size = this->size - start;
		}

		if (start + size > this->size)
		{
			throw std::runtime_error("Buffer mapping is out of bounds.");
		}

		mappedData = (T*)glMapNamedBufferRange(bufferId, start * sizeof(T), size * sizeof(T), access);

		mappingStart = start;
		mappingSize = size;
	}

	/**
	 * \brief Unmaps the buffer from the CPU. This also updates the buffer on the GPU.
	 */
	void unmapBuffer()
	{
		if (mappedData == nullptr)
		{
			return;
		}

		if(!glUnmapNamedBuffer(bufferId))
		{
			throw std::runtime_error("Buffer unmapping failed.");
		}

		mappedData = nullptr;
		mappingStart = 0;
		mappingSize = 0;
	}
	
	/**
	 * \brief Returns a reference to the element at the given index. Must be mapped accordingly with mapBuffer. (Read or write access)
	 * 
	 * \param index - The index of the element to get.
	 * \return A reference to the element at the given index.
	 */
	T& operator[](unsigned int index)
	{
		if (mappedData == nullptr)
		{
			throw std::runtime_error("Buffer must be mapped before accessing it.");
		}

		if (index < mappingStart || index >= mappingStart + mappingSize)
		{
			throw std::runtime_error("Index is out of mapping bounds.");
		}

		return mappedData[index - mappingStart];
	}

	/**
	 * \brief Returns a reference to the element at the given index. Must be mapped accordingly with mapBuffer. (Read)
	 *
	 * \param index - The index of the element to get.
	 * \return A reference to the element at the given index.
	 */
	const T& operator[](unsigned int index) const
	{
		if (mappedData == nullptr)
		{
			throw std::runtime_error("Buffer must be mapped before accessing it.");
		}

		if (index < mappingStart || index >= mappingStart + mappingSize)
		{
			throw std::runtime_error("Index is out of mapping bounds.");
		}

		return mappedData[index - mappingStart];
	}

	GLuint getBufferId() const
	{
		return bufferId;
	}

	/**
	 * \brief Binds the whole buffer to the given binding point.
	 * 
	 * \param bindingId - The binding point to bind the buffer to.
	 */
	void bindBuffer(GLuint bindingId) const
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingId, bufferId);
	}

	/**
	 * \brief Binds a part of the buffer to the given binding point.
	 * 
	 * \param bindingId - The binding point to bind the buffer to.
	 * \param offset - The offset of the buffer to bind in sizeof(T) units.
	 * \param size - The size of the buffer to bind in sizeof(T) units.
	 */
	void bindBuffer(GLuint bindingId, GLuint offset, GLuint size) const
	{
		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingId, bufferId, offset * sizeof(T), size * sizeof(T));
	}

private:
	GLuint bufferId = 0;
	unsigned int size = 0;
	T* mappedData = nullptr;
	unsigned int mappingStart = 0;
	unsigned int mappingSize = 0;
	const GLenum usage = 0;
	RenderEngine& renderEngine;
};

template <typename T>
using ssbo_ptr = std::shared_ptr<StorageBuffer<T>>;
template <typename T>
inline ssbo_ptr<T> make_ssbo(unsigned int size, GLenum usage)
{
	return std::make_shared<StorageBuffer<T>>(size, usage);
}

} // namespace renderer
