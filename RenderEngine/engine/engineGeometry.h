#pragma once

#include <glm/glm.hpp>
#include <vector>


struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texture;
};


struct InputGeometryArray {
	std::vector<Vertex> vertexes;
	bool hasIndexes = false;
	std::vector<unsigned int> indexes;
	int arraySize;
	std::vector<glm::vec4> ambientColor;
	std::vector<glm::vec3> position;
	int renderAs;
};

