#include "geometry.h"
#include <spdlog/spdlog.h>
#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

using namespace renderer;

renderer::Geometry::Geometry(GLenum drawType, int vertexNum) : drawType(drawType), vertexNum(vertexNum)
{
	glGenVertexArrays(1, &vaoId);
	spdlog::debug("Geometry created with id: {}", vaoId);
}

void renderer::Geometry::bindVao() const
{
	glBindVertexArray(vaoId);
}

void renderer::Geometry::unbindVao() const
{
	glBindVertexArray(0);
}

void renderer::Geometry::setVertexNum(int vertexNum)
{
	if (indexBufferId)
	{
		throw std::runtime_error("Cannot set vertex number when index buffer is present");
	}
	this->vertexNum = vertexNum;
}

size_t renderer::Geometry::getVboDataNum(GLuint id) const
{
	if(!vboSizes.contains(id))
	{
		throw std::runtime_error("VBO not found");
	}
	return vboSizes.find(id)->second.dataNum;
}

void renderer::Geometry::createIndexBuffer(const std::vector<unsigned int>& indices)
{
	bindVao();
	GLuint vboId;
	glGenBuffers(1, &vboId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
	unbindVao();
	indexBufferId = vboId;
	vertexNum = indices.size();
	spdlog::debug("Index buffer created with id: {} for geometry with id: {}", vboId, vaoId);
}

void renderer::Geometry::draw() const
{
	bindVao();
	if (indexBufferId)
	{
		glDrawElements(drawType, vertexNum, GL_UNSIGNED_INT, 0);
	}
	else
	{
		glDrawArrays(drawType, 0, vertexNum);
	}
	unbindVao();
}

renderer::Geometry::~Geometry()
{
	bindVao();
	for(auto& vbo : vboSizes)
	{
		glDeleteBuffers(1, &vbo.first);
	}
	if(indexBufferId)
	{
		glDeleteBuffers(1, &indexBufferId.value());
	}
	glDeleteVertexArrays(1, &vaoId);
	spdlog::debug("Geometry deleted with id: {}", vaoId);
}

void renderer::BasicGeometryArray::updateActiveInstanceParams()
{
	if (reuploadRequired)
	{
		if (instanceOffsetVboId == 0)
		{
			std::vector<ArrayAttribute> attributes = { ArrayAttribute{10, 4, GL_FLOAT, NULL} };
			instanceOffsetVboId = createVboPerInstance(offsets, attributes, true);
		}
		else
		{
			geometry->reUploadVbo(instanceOffsetVboId, offsets);
		}
		if (instanceColorVboId == 0)
		{
			std::vector<ArrayAttribute> attributes = { ArrayAttribute{11, 4, GL_FLOAT, NULL} };
			instanceColorVboId = createVboPerInstance(colors, attributes, true);
		}
		else
		{
			geometry->reUploadVbo(instanceColorVboId, colors);
		}
	}
	else
	{
		if(colorsNeedUpdate)
		{
			geometry->updateVbo(instanceColorVboId, colors, instancesToDraw);
		}
		if (offsetsNeedUpdate)
		{
			geometry->updateVbo(instanceOffsetVboId, offsets, instancesToDraw);
		}
	}
	colorsNeedUpdate = false;
	offsetsNeedUpdate = false;
	reuploadRequired = false;
}

void renderer::BasicGeometryArray::setMaxInstanceNum(size_t instanceNum)
{
	if (instanceNum != colors.size())
	{
		std::vector<glm::vec4> newColors(instanceNum);
		for (size_t i = 0; i < colors.size() && i < instanceNum; i++)
		{
			newColors[i] = colors[i];
		}
		colors = std::move(newColors);

		std::vector<glm::vec4> newPositions(instanceNum);
		for (size_t i = 0; i < offsets.size() && i < instanceNum; i++)
		{
			newPositions[i] = offsets[i];
		}
		offsets = std::move(newPositions);

		instancesToDraw = instanceNum;

		reuploadRequired = true;
	}
}

void renderer::BasicGeometryArray::setMaxInstanceNum(size_t instanceNum, std::vector<glm::vec4>&& positions, std::vector<glm::vec4>&& colors)
{
	if (instanceNum != colors.size() || instanceNum != positions.size())
	{
		throw std::runtime_error("Instance number is not equal to colors or positions size");
	}
	instancesToDraw = instanceNum;
	reuploadRequired = true;
	this->colors = std::move(colors);
	this->offsets = std::move(positions);
}

void renderer::BasicGeometryArray::setActiveInstanceNum(size_t instanceNum)
{
	if(instanceNum > colors.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	if (instanceNum > instancesToDraw)
	{
		colorsNeedUpdate = true;
		offsetsNeedUpdate = true;
	}
	instancesToDraw = instanceNum;
}

void renderer::BasicGeometryArray::setColor(size_t instanceId, const glm::vec4& color)
{
	if (instanceId >= colors.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	colors[instanceId] = color;
	if(instancesToDraw > instanceId)
		colorsNeedUpdate = true;
}

void renderer::BasicGeometryArray::setOffset(size_t instanceId, const glm::vec4& offset)
{
	if (instanceId >= offsets.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	offsets[instanceId] = offset;
	if (instancesToDraw > instanceId)
		offsetsNeedUpdate = true;
}

const glm::vec4& renderer::BasicGeometryArray::getColor(size_t instanceId) const
{
	if (instanceId >= colors.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	return colors[instanceId];
}

const glm::vec4& renderer::BasicGeometryArray::getOffset(size_t instanceId) const
{
	if (instanceId >= offsets.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	return offsets[instanceId];
}

void renderer::BasicPosGeometryArray::updateActiveInstanceParams()
{
	if (reuploadRequired)
	{
		if (instanceOffsetVboId == 0)
		{
			std::vector<ArrayAttribute> attributes = { ArrayAttribute{10, 4, GL_FLOAT, NULL} };
			instanceOffsetVboId = createVboPerInstance(offsets, attributes, true);
		}
		else
		{
			geometry->reUploadVbo(instanceOffsetVboId, offsets);
		}
	}
	else
	{
		if (offsetsNeedUpdate)
		{
			geometry->updateVbo(instanceOffsetVboId, offsets, instancesToDraw);
		}
	}
	offsetsNeedUpdate = false;
	reuploadRequired = false;
}

void renderer::BasicPosGeometryArray::setMaxInstanceNum(size_t instanceNum)
{
	if (instanceNum != offsets.size())
	{
		std::vector<glm::vec4> newPositions(instanceNum);
		for (size_t i = 0; i < offsets.size() && i < instanceNum; i++)
		{
			newPositions[i] = offsets[i];
		}
		offsets = std::move(newPositions);

		instancesToDraw = instanceNum;

		reuploadRequired = true;
	}
}

void renderer::BasicPosGeometryArray::setMaxInstanceNum(size_t instanceNum, std::vector<glm::vec4>&& positions)
{
	if (instanceNum != positions.size())
	{
		throw std::runtime_error("Instance number is not equal to colors or positions size");
	}
	instancesToDraw = instanceNum;
	reuploadRequired = true;
	this->offsets = std::move(positions);
}

void renderer::BasicPosGeometryArray::setActiveInstanceNum(size_t instanceNum)
{
	if (instanceNum > offsets.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	if (instanceNum > instancesToDraw)
	{
		offsetsNeedUpdate = true;
	}
	instancesToDraw = instanceNum;
}

void renderer::BasicPosGeometryArray::setOffset(size_t instanceId, const glm::vec4& offset)
{
	if (instanceId >= offsets.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	offsets[instanceId] = offset;
	if (instancesToDraw > instanceId)
		offsetsNeedUpdate = true;
}

const glm::vec4& renderer::BasicPosGeometryArray::getOffset(size_t instanceId) const
{
	if (instanceId >= offsets.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	return offsets[instanceId];
}

void renderer::ParticleGeometryArray::updateActiveInstanceParams()
{
	if (reuploadRequired)
	{
		if (instanceOffsetVboId == 0)
		{
			std::vector<ArrayAttribute> attributes = { ArrayAttribute{10, 4, GL_FLOAT, NULL} };
			instanceOffsetVboId = createVboPerInstance(offsets, attributes, true);
		}
		else
		{
			geometry->reUploadVbo(instanceOffsetVboId, offsets);
		}
		if (instanceIdVboId == 0)
		{
			std::vector<ArrayAttribute> attributes = { ArrayAttribute{11, 1, GL_INT, NULL} };
			instanceIdVboId = createVboPerInstance(ids, attributes, true, true);
		}
		else
		{
			geometry->reUploadVbo(instanceIdVboId, ids);
		}
		if (instanceSpeedVboId == 0)
		{
			std::vector<ArrayAttribute> attributes = { ArrayAttribute{12, 1, GL_FLOAT, NULL} };
			instanceSpeedVboId = createVboPerInstance(speeds, attributes, true);
		}
		else
		{
			geometry->reUploadVbo(instanceSpeedVboId, speeds);
		}
	}
	else
	{
		if (offsetsNeedUpdate)
		{
			geometry->updateVbo(instanceOffsetVboId, offsets, instancesToDraw);
		}
		if (idsNeedUpdate)
		{
			geometry->updateVbo(instanceIdVboId, ids, instancesToDraw);
		}
		if (speedsNeedUpdate)
		{
			geometry->updateVbo(instanceSpeedVboId, speeds, instancesToDraw);
		}
	}
	offsetsNeedUpdate = false;
	idsNeedUpdate = false;
	speedsNeedUpdate = false;
	reuploadRequired = false;
}

void renderer::ParticleGeometryArray::setMaxInstanceNum(size_t instanceNum)
{
	if (instanceNum != offsets.size())
	{
		std::vector<glm::vec4> newOffsets(instanceNum);
		for (size_t i = 0; i < offsets.size() && i < instanceNum; i++)
		{
			newOffsets[i] = offsets[i];
		}
		offsets = std::move(newOffsets);

		std::vector<int> newIds(instanceNum);
		for (size_t i = 0; i < ids.size() && i < instanceNum; i++)
		{
			newIds[i] = ids[i];
		}
		ids = std::move(newIds);

		std::vector<float> newSpeeds(instanceNum);
		for (size_t i = 0; i < speeds.size() && i < instanceNum; i++)
		{
			newSpeeds[i] = speeds[i];
		}
		speeds = std::move(newSpeeds);

		instancesToDraw = instanceNum;

		reuploadRequired = true;
	}
}

void renderer::ParticleGeometryArray::setActiveInstanceNum(size_t instanceNum)
{
	if (instanceNum > offsets.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	if (instanceNum > instancesToDraw)
	{
		offsetsNeedUpdate = true;
		idsNeedUpdate = true;
		speedsNeedUpdate = true;
	}
	instancesToDraw = instanceNum;
}

void renderer::ParticleGeometryArray::setOffset(size_t instanceId, const glm::vec4& offset)
{
	if (instanceId >= offsets.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	offsets[instanceId] = offset;
	if (instancesToDraw > instanceId)
		offsetsNeedUpdate = true;
}

const glm::vec4& renderer::ParticleGeometryArray::getOffset(size_t instanceId) const
{
	if (instanceId >= offsets.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	return offsets[instanceId];
}

void renderer::ParticleGeometryArray::setId(size_t instanceId, int id)
{
	if (instanceId >= ids.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	ids[instanceId] = id;
	if (instancesToDraw > instanceId)
		idsNeedUpdate = true;
}

int renderer::ParticleGeometryArray::getId(size_t instanceId) const
{
	if (instanceId >= ids.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	return ids[instanceId];
}

void renderer::ParticleGeometryArray::setSpeed(size_t instanceId, float speed)
{
	if (instanceId >= speeds.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	speeds[instanceId] = speed;
	if (instancesToDraw > instanceId)
		speedsNeedUpdate = true;
}

float renderer::ParticleGeometryArray::getSpeed(size_t instanceId) const
{
	if (instanceId >= speeds.size())
	{
		throw std::runtime_error("Instance number is bigger than max instance number");
	}
	return speeds[instanceId];
}

renderer::GeometryArray::GeometryArray(std::shared_ptr<Geometry> geometry)
{
	this->geometry = geometry;
}

int renderer::GeometryArray::getActiveInstanceNum() const
{
	return instancesToDraw;
}

void renderer::GeometryArray::draw() const
{
	geometry->bindVao();
	if (geometry->indexBufferId)
	{
		glDrawElementsInstanced(geometry->drawType, geometry->vertexNum, GL_UNSIGNED_INT, 0, instancesToDraw);
	}
	else
	{
		glDrawArraysInstanced(geometry->drawType, 0, geometry->vertexNum, instancesToDraw);
	}
	geometry->unbindVao();
}

void renderer::InstancedGeometry::setInstanceNum(size_t instanceNum)
{
	this->instancesToDraw = instanceNum;
}

renderer::MeshGeometry::MeshGeometry(GLenum drawType, const std::vector<BasicVertex>& vertices, const std::vector<unsigned int>& indices)
	: Geometry(drawType, indices.size())
{
	createVbo(vertices, { {0, 4, GL_FLOAT, offsetof(BasicVertex, position)},
								  {1, 4, GL_FLOAT, offsetof(BasicVertex, normal)},
								  {2, 2, GL_FLOAT, offsetof(BasicVertex, texCoord)} });
	createIndexBuffer(indices);
}

namespace std
{
template<> struct hash<BasicVertex>
{
	size_t operator()(BasicVertex const& vertex) const
	{
		return ((hash<glm::vec4>()(vertex.position) ^
			(hash<glm::vec4>()(vertex.normal) << 1)) >> 1) ^
			(hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};
}

std::shared_ptr<MeshGeometry> renderer::loadGeometry(const std::string& path)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
	{
		throw std::runtime_error("Failed to load obj file: " + err);
	}
	std::vector<BasicVertex> vertices;
	std::vector<unsigned int> indices;
	for (const auto& shape : shapes)
	{
		std::unordered_map<BasicVertex, unsigned int> uniqueVertices;
		for (const auto& index : shape.mesh.indices)
		{
			BasicVertex vertex = {};
			vertex.position = { attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1], attrib.vertices[3 * index.vertex_index + 2], 1.0f };
			vertex.normal = { attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1], attrib.normals[3 * index.normal_index + 2], 0.0f };
			vertex.texCoord = { attrib.texcoords[2 * index.texcoord_index + 0], attrib.texcoords[2 * index.texcoord_index + 1] };
			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<unsigned int>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}
	return std::make_shared<MeshGeometry>(GL_TRIANGLES, vertices, indices);
}
