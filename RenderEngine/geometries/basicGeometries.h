#pragma once

#include "geometry.h"

namespace renderer
{

class Circle : public Geometry {
public:
	/**
	 * \brief Creates a circle with a radius of 1 in the origo of the z = 0 plane
	 * \param vertexNum The number of vertices to use for the circle
	 */
	Circle(int vertexNum);
};

class Square : public Geometry
{
public:
	/**
	 * \brief Creates a square with side length of 1 and the center in the origo of the z=0 plane.
	 */
	Square();
};

class Cube : public Geometry
{
public:
	/**
	 * \brief Creates a cube with side length of 1 and the center in the origo.
	 */
	Cube();
};

class Sphere : public Geometry
{
public:
	/**
	 * \brief Creates a sphere with radius of 1 and the center in the origo.
	 * \param vertexNum The number of vertices to use for the sphere
	 */
	Sphere(int vertexNum);
};

} // namespace renderer

