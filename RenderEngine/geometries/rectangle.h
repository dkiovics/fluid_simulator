#pragma once

#include "geometry.h"


class Rectangle : public Geometry {
public:
	Rectangle(int instanceCount, std::vector<glm::vec3> position, std::vector<glm::vec4> color, std::shared_ptr<RenderEngine> engine, float a, float b, float c);

protected:
	virtual InputGeometryArray generateGeometry() const;

private:
	const float a, b, c;
};

