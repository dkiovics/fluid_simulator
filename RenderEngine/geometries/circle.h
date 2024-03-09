#pragma once

#include "geometry.h"


class Circle : public Geometry {
public:
	Circle(int instanceCount, std::vector<glm::vec3> position, std::vector<glm::vec4> color, std::shared_ptr<renderer::RenderEngine> engine, float radius, int vertexNum);

protected:
	virtual InputGeometryArray generateGeometry() const;

private:
	const float radius;
	const int vertexNum;
};

