#pragma once

#include <glm/glm.hpp>

#include "../engine/renderEngine.h"
#include "../engine/engineGeometry.h"

constexpr auto PI = 3.14159265359f;


class Geometry {
public:
	Geometry(int instanceCount, std::vector<glm::vec3>& position, std::vector<glm::vec4>& color, std::shared_ptr<RenderEngine> engine);

	Geometry(const Geometry&) = delete;
	Geometry(Geometry&&) = delete;
	Geometry& operator=(const Geometry&) = delete;
	Geometry& operator=(Geometry&&) = delete;

	void draw(int num = -1) const;

	inline glm::vec3& positionAt(const int pos) {
		return position[pos];
	}
	inline glm::vec4& colorAt(const int pos) {
		return color[pos];
	}
	inline int getInstanceCount() const {
		return instanceCount;
	}

	void updateColor(int num = -1);
	void updatePosition(int num = -1);

	virtual ~Geometry();

public:
	const std::shared_ptr<RenderEngine> engine;

private:
	std::vector<glm::vec3> position;
	std::vector<glm::vec4> color;

	const int instanceCount;
	unsigned int geometryId;
	
protected:
	unsigned int getGeometryId() const;

	virtual InputGeometryArray generateGeometry() const = 0;

	void initGeometry();

};

