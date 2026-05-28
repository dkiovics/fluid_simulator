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

class FlipSquare : public Geometry
{
public:
	/**
	 * \brief Creates a square with side length of 1 and the center in the origo of the z=0 plane. The texture coordinates are flipped along the y-axis.
	 */
	FlipSquare();
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

class Arrow4 : public Geometry
{
public:
	/**
	 * \brief Creates a 4 sided arrow pointing upwards and with its bottom in the origo.
	 */
	Arrow4(float width, float baseHeight, float tipHeight);
};

class Line : public Geometry
{
public:
	/**
	 * \brief Creates a line between two points in 3D space.
	 * \param p1 The first point of the line
	 * \param p2 The second point of the line
	 * \param lineWidth The width of the line
	 */
	Line(glm::vec3 p1, glm::vec3 p2, float lineWidth);

	void draw() const override;

	float lineWidth;
};

} // namespace renderer

