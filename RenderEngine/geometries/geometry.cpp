#include "geometry.h"


Geometry::Geometry(int instanceCount, std::vector<glm::vec3>& position, std::vector<glm::vec4>& color, std::shared_ptr<renderer::RenderEngine> engine)
				: position(position), color(color), engine(engine), instanceCount(instanceCount) {
	if (instanceCount != position.size() || instanceCount != color.size())
		throw std::logic_error("Inconsistent geometry input array sizes");
}

void Geometry::draw(int num) const {
	if(num == -1)
		engine->drawGeometryArray(getGeometryId());
	else
		engine->drawGeometryArray(getGeometryId(), num);
}

void Geometry::updateColor(int num) {
	engine->updateColor(getGeometryId(), color, num);
}

void Geometry::updatePosition(int num) {
	engine->updatePosition(getGeometryId(), position, num);
}

Geometry::~Geometry() {
	engine->removeGeometryArray(getGeometryId());
}

unsigned int Geometry::getGeometryId() const {
	return geometryId;
}

void Geometry::initGeometry() {
	InputGeometryArray input = generateGeometry();
	input.ambientColor = color;
	input.arraySize = instanceCount;
	input.position = position;
	geometryId = engine->createGeometryArray(input);
}
