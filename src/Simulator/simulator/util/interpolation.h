#pragma once

#include <glm/glm.hpp>
#include <stdexcept>
#include <math.h>

inline double trilinearInterpoll(const glm::dvec3& center, const glm::dvec3& pos, const glm::dvec3 invCellD)
{
	glm::dvec3 vector = (pos - center) * invCellD;
	double x = (1.0 - std::fabs(vector.x));
	double y = (1.0 - std::fabs(vector.y));
	double z = (1.0 - std::fabs(vector.z));
#ifdef DEBUG_MODE
	if (x < -1e-6 || y < -1e-6 || z < -1e-6)
		throw std::runtime_error("Trilinear interpollation component is negative");
#endif
	return x * y * z;
}


inline glm::dvec3 trilinearInterpollGradient(const glm::dvec3& center, const glm::dvec3& pos, const glm::dvec3 invCellD)
{
	glm::dvec3 vector = (pos - center) * invCellD;
	double xabs = (1.0 - std::fabs(vector.x));
	double yabs = (1.0 - std::fabs(vector.y));
	double zabs = (1.0 - std::fabs(vector.z));
	double xsign = vector.x > 0.0 ? -1.0 : 1.0;																//0 esetén nincs gradiens -> 0 a C
	double ysign = vector.y > 0.0 ? -1.0 : 1.0;
	double zsign = vector.z > 0.0 ? -1.0 : 1.0;
#ifdef DEBUG_MODE
	if (xabs < -1e-6 || yabs < -1e-6 || zabs < -1e-6)
		throw std::runtime_error("Trilinear interpollation gradient component is negative");
#endif
	return glm::dvec3(xsign * yabs * zabs, ysign * xabs * zabs, zsign * xabs * yabs) * invCellD;
}

