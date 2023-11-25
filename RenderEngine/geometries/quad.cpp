#include "quad.h"

Quad::Quad(int instanceCount, std::vector<glm::vec3> position, std::vector<glm::vec4> color, std::shared_ptr<RenderEngine> engine, float a, float b)
	: a(a/2), b(b/2), vertexNum(vertexNum), Geometry(instanceCount, position, color, engine) {
	initGeometry();
}

InputGeometryArray Quad::generateGeometry() const {
	InputGeometryArray input;
	input.hasIndexes = true;
	input.renderAs = GL_TRIANGLES;

	input.vertexes.push_back({
		{-a,-b,0},
		{0,0,1},
		{0,0}
		});
	input.vertexes.push_back({
		{-a,b,0},
		{0,0,1},
		{0,1}
		});
	input.vertexes.push_back({
		{a,-b,0},
		{0,0,1},
		{1,0}
		});
	input.vertexes.push_back({
		{a,b,0},
		{0,0,1},
		{1,1}
		});

	input.indexes.push_back(0);
	input.indexes.push_back(1);
	input.indexes.push_back(2);
	input.indexes.push_back(1);
	input.indexes.push_back(2);
	input.indexes.push_back(3);

	return input;
}
