#pragma once

#include <glm/glm.hpp>


namespace genericfsim::particles {

struct Particle {
	glm::dvec3 pos;
	glm::dvec3 v;
	glm::dvec3 c[3];
};

}