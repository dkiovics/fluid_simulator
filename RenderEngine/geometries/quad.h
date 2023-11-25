#pragma once

#include "geometry.h"


class Quad : public Geometry {
public:
	Quad(int instanceCount, std::vector<glm::vec3> position, std::vector<glm::vec4> color, std::shared_ptr<RenderEngine> engine, float a, float b);

protected:
	virtual InputGeometryArray generateGeometry() const;

private:
	const float a, b;
	const int vertexNum;
};
