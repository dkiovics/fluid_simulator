#pragma once

#include "../engine/renderEngine.h"
#include <vector>
#include <map>
#include <stdexcept>
#include <optional>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "../glad/glad.h"
#include <spdlog/spdlog.h>

constexpr auto PI = 3.14159265359f;

namespace renderer
{

/**
* \brief Config struct of a VBO attribute
* \param id - The id of the attribute
* \param size - The number of components in the attribute, can be 1, 2, 3, 4
* \param type - The type of the attribute, can be GL_FLOAT, GL_INT, etc.
* \param offset - The offset of the attribute in the data
*/
struct ArrayAttribute
{
	ArrayAttribute(GLuint id, GLint size, GLenum type, size_t offset) : id(id), size(size), type(type), offset(offset) {}
	GLuint id;
	GLint size;
	GLenum type;
	size_t offset;
};

struct BasicVertex
{
	glm::vec4 position;
	glm::vec4 normal;
	glm::vec2 texCoord;
};

class Drawable
{
public:
	virtual void draw() const = 0;
};

class Geometry : public Drawable
{
public:
	/**
	 * \brief Constructor
	 * \param drawType - The draw type of the geometry, can be GL_TRIANGLES, GL_LINES, etc.
	 * \param vertexNum - The number of vertices in the geometry, if the geometry has an index buffer, this parameter is ignored
	 */
	Geometry(GLenum drawType, int vertexNum = 0);

	Geometry(const Geometry&) = delete;
	Geometry& operator=(const Geometry&) = delete;
	Geometry(Geometry&&) = delete;
	Geometry& operator=(Geometry&&) = delete;

	virtual ~Geometry();

	/**
	 * \brief Draws the geometry
	 */
	void draw() const override;

protected:
	/**
	 * \brief Creates a VBO and returns its id
	 * \param data - The data to be stored in the VBO
	 * \param attributes - The attributes of the data
	 * \param isDynamic - Whether the data will be updated frequently
	 * \return The id of the created VBO
	 */
	template<typename T>
	GLuint createVbo(const std::vector<T>& data, const std::vector<ArrayAttribute>& attributes, bool isDynamic = false)
	{
		bindVao();
		GLuint vboId;
		VboData vboData(data.size(), isDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
		glGenBuffers(1, &vboId);
		glBindBuffer(GL_ARRAY_BUFFER, vboId);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), vboData.drawType);
		for (auto& attrib : attributes)
		{
			glEnableVertexAttribArray(attrib.id);
			glVertexAttribPointer(attrib.id, attrib.size, attrib.type, GL_FALSE, sizeof(T), (void*)attrib.offset);
		}
		vboSizes.insert(std::make_pair(vboId, vboData));
		unbindVao();
		return vboId;

		spdlog::debug("VBO created with id: {} for geometry with id: {}", vboId, vaoId);
	}

	/**
	 * \brief Reuploads the data of a VBO, the new data might have a different size
	 * \param id - The id of the VBO
	 * \param data - The new data
	 */
	template<typename T>
	void reUploadVbo(GLuint id, const std::vector<T>& data)
	{
		auto vboData = vboSizes.find(id);
		if (vboData == vboSizes.end())
		{
			throw std::runtime_error("VBO with id " + std::to_string(id) + " does not exist");
		}
		bindVao();
		glBindBuffer(GL_ARRAY_BUFFER, id);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), vboData->second.drawType);
		vboData->second.dataNum = data.size();
		unbindVao();
	}

	/**
	 * \brief Updates the data of a VBO, can be a partial update
	 * \param id - The id of the VBO
	 * \param data - The new data, should not exceed the original data size
	 * \param elementNum - The number of elements in the data, if not specified,
	 * the number of elements is the same as the original data size
	 */
	template<typename T>
	void updateVbo(GLuint id, const std::vector<T>& data, size_t elementNum = 0) const
	{
		const auto& vboData = vboSizes.find(id);
		if (vboData == vboSizes.end())
		{
			throw std::runtime_error("VBO with id " + std::to_string(id) + " does not exist");
		}
		if (data.size() > vboData->second.dataNum)
		{
			throw std::runtime_error("Data size exceeds the original data size");
		}
		bindVao();
		glBindBuffer(GL_ARRAY_BUFFER, id);
		glBufferSubData(GL_ARRAY_BUFFER, 0, (elementNum == 0 ? data.size() : elementNum) * sizeof(T), data.data());
		unbindVao();
	}

	/**
	 * \brief Returns the number of data in a VBO
	 * \param id - The id of the VBO
	 * \return The number of data in the VBO
	 */
	size_t getVboDataNum(GLuint id) const;

	friend class GeometryArray;
	friend class BasicGeometryArray;
	friend class BasicPosGeometryArray;

	/**
	 * \brief Binds the VAO of the geometry
	 */
	void bindVao() const;

	/**
	 * \brief Unbinds the VAO of the geometry
	 */
	void unbindVao() const;

	/**
	* \brief Sets the number of vertices in the geometry, if the geometry has an index buffer, this parameter is ignored
	* \param vertexNum - The number of vertices
	*/
	void setVertexNum(int vertexNum);

	/**
	 * \brief Creates an index buffer
	 * \param indices - The indices of the geometry
	 */
	void createIndexBuffer(const std::vector<unsigned int>& indices);

	std::optional<GLuint> indexBufferId;

private:
	/**
	 * \brief Config struct of a VBO
	 * \param dataNum - The number of data in the VBO
	 * \param drawType - The draw type of the data, can be GL_STATIC_DRAW, GL_DYNAMIC_DRAW, etc.
	 */
	struct VboData
	{
		VboData(size_t dataNum, GLenum drawType) : dataNum(dataNum), drawType(drawType) {}
		size_t dataNum;
		GLenum drawType;
	};

	GLenum drawType;
	int vertexNum;
	GLuint vaoId = 0;
	std::map<GLuint, VboData> vboSizes;

};

class GeometryArray : public Drawable
{
public:
	GeometryArray(std::shared_ptr<Geometry> geometry);

	GeometryArray(const GeometryArray&) = delete;
	GeometryArray& operator=(const GeometryArray&) = delete;
	GeometryArray(GeometryArray&&) = delete;
	GeometryArray& operator=(GeometryArray&&) = delete;

	/**
	 * \brief Draws the geometry array
	 */
	void draw() const override;

	/**
	 * \brief Returns the number of instances to draw
	 * \return The number of instances to draw
	 */
	int getActiveInstanceNum() const;

protected:
	std::shared_ptr<Geometry> geometry;

	int instancesToDraw = 0;

	/**
	 * \brief Creates a per instance VBO and returns its id
	 * \param data - The data to be stored in the VBO
	 * \param attributes - The attributes of the data
	 * \param isDynamic - Whether the data will be updated frequently
	 * \return The id of the created VBO
	 */
	template<typename T>
	GLuint createVboPerInstance(const std::vector<T>& data, const std::vector<ArrayAttribute>& attributes, bool isDynamic = false)
	{
		GLuint vboId = geometry->createVbo(data, attributes, isDynamic);
		geometry->bindVao();
		for(auto& attrib : attributes)
		{
			glVertexAttribDivisor(attrib.id, 1);
		}
		geometry->unbindVao();
		return vboId;

		spdlog::debug("Per instance VBO created with id: {}", vboId);
	}
};

class BasicGeometryArray : public GeometryArray
{
public:
	using GeometryArray::GeometryArray;

	/**
	 * \brief Resizes the number of instances, the initial instance number is 0
	 * \param instanceNum - The new number of instances
	 */
	void setMaxInstanceNum(size_t instanceNum);

	/**
	 * \brief Resizes the number of instances, the number of positions and colors must be equal to the new instance number.
	 * The initial instance number is 0
	 * \param instanceNum - The new number of instances
	 * \param positions - The positions of the instances
	 * \param colors - The colors of the instances
	 */
	void setMaxInstanceNum(size_t instanceNum, std::vector<glm::vec4>&& positions, std::vector<glm::vec4>&& colors);
	
	/**
	 * \brief Updates the positions and colors of the instances
	 */
	void updateActiveInstanceParams();

	/**
	 * \brief Sets the active instance number, this is the number of instances to draw,
	 * the active instance number should not exceed the max instance number
	 * \param instanceNum - The new active instance number
	 */
	void setActiveInstanceNum(size_t instanceNum);

	void setColor(size_t index, const glm::vec4& color);
	void setOffset(size_t index, const glm::vec4& offset);

	const glm::vec4& getColor(size_t index) const;
	const glm::vec4& getOffset(size_t index) const;

private:
	GLuint instancePosVboId = 0;
	GLuint instanceColorVboId = 0;

	bool colorsNeedUpdate = false;
	bool positionsNeedUpdate = false;

	bool reuploadRequired = false;

	std::vector<glm::vec4> positions;
	std::vector<glm::vec4> colors;
};

class BasicPosGeometryArray : public GeometryArray
{
public:
	using GeometryArray::GeometryArray;

	/**
	 * \brief Resizes the number of instances, the initial instance number is 0
	 * \param instanceNum - The new number of instances
	 */
	void setMaxInstanceNum(size_t instanceNum);

	/**
	 * \brief Resizes the number of instances, the number of positions must be equal to the new instance number.
	 * The initial instance number is 0
	 * \param instanceNum - The new number of instances
	 * \param positions - The positions of the instances
	 */
	void setMaxInstanceNum(size_t instanceNum, std::vector<glm::vec4>&& positions);

	/**
	 * \brief Updates the positions of the instances
	 */
	void updateActiveInstanceParams();

	/**
	 * \brief Sets the active instance number, this is the number of instances to draw,
	 * the active instance number should not exceed the max instance number
	 * \param instanceNum - The new active instance number
	 */
	void setActiveInstanceNum(size_t instanceNum);

	void setOffset(size_t index, const glm::vec4& offset);

	const glm::vec4& getOffset(size_t index) const;

private:
	GLuint instancePosVboId = 0;

	bool positionsNeedUpdate = false;

	bool reuploadRequired = false;

	std::vector<glm::vec4> positions;
};

} // namespace renderer

