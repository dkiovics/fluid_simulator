#pragma once

#include <glm/glm.hpp>
#include <functional>

namespace glm {

/**
 * Executes a given lambda on each component of a vector and returns a new vector.
 * 
 * \param v - a vector
 * \param lambda - a lambda that takes an axis index (0-based) and a vector component and returns the new component
 * \return - the new vector
 */
template<int P, typename T>
inline vec<P, T> forEachTransform(const vec<P, T>& v, std::function <T(int, T)> lambda) {
	vec<P, T> result;
	for (int p = 0; p < P; p++) {
		result[p] = lambda(p, v[p]);
	}
	return result;
}

/**
 * Executes a given lambda on each component of a vector.
 *
 * \param v - a vector
 * \param lambda - a lambda that takes an axis index (0-based) and a vector component
 */
template<int P, typename T>
inline void forEachComponent(const vec<P, T>& v, std::function <void(int, T)> lambda) {
	for (int p = 0; p < P; p++) {
		lambda(p, v[p]);
	}
}

/**
 * Converts a vec3 to dvec3.
 * 
 * \param v - the vec3
 * \return - the dvec3
 */
template<typename T>
inline dvec3 toDvec3(const vec<3, T>& v) {
	return dvec3(v.x, v.y, v.z);
}

/**
 * Converts a vec2 to dvec2.
 *
 * \param v - the vec2
 * \return - the dvec2
 */
template<typename T>
inline dvec2 toDvec2(const vec<2, T>& v) {
	return dvec2(v.x, v.y);
}

/**
 * Converts a vec3 to ivec3.
 *
 * \param v - the vec3
 * \return - the ivec3
 */
template<typename T>
inline ivec3 toIvec3(const vec<3, T>& v) {
	return ivec3(v.x, v.y, v.z);
}

/**
 * Converts a vec2 to ivec2.
 *
 * \param v - the vec3
 * \return - the ivec3
 */
template<typename T>
inline ivec2 toIvec2(const vec<2, T>& v) {
	return ivec2(v.x, v.y);
}

/**
 * Converts a vec3 to vec3.
 * *
 * \param v - the dvec3
 * * \return - the vec3
 * */
template<typename T>
inline vec3 toVec3(const vec<3, T>& v) {
	return vec3(v.x, v.y, v.z);	
}

/**
 * Converts a vec2 to vec2.
 * *
 * \param v - the dvec2
 * * \return - the vec2
 * */
template<typename T>
inline vec2 toVec2(const vec<2, T>& v) {
	return vec2(v.x, v.y);
}

}


