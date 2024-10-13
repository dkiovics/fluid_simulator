#pragma once

#include <glm/glm.hpp>
#include <array>
#include <vector>
#include <functional>
#include <utility>
#include <atomic>
#include "macGridCell.h"
#include "obstacles.hpp"
#include "../util/glmExtraOps.h"


namespace genericfsim::macgrid {

/**
 * A class that implements a MAC grid.
 */
class MacGrid {									//TODO: make the grid cells a 1D array
public:
	/**
	 * Contructs the MAC grid class.
	 * 
	 * \param targetDimensions - size of the grid in each axis (targeted size, might be smaller)
	 * \param resolution - the number of cells per dimension entity, has priority over targetDimensions
	 * \param twoD - the grid beacomes 2D (in the z direction it only consists of 3 cells)
	 */
	MacGrid(glm::dvec3 targetDimensions, double resolution, bool twoD);

	/**
	 * Returns all 8 faces closest to a point in space.
	 * 
	 * \param pos - a point in space
	 * \param axis - the axis of the returned faces
	 * \return - an array for the given axis, which consists of 8 faces
	 */
	std::array<std::array<MacGridCell::FaceRef, 8>, 3> getFacesAround(const glm::dvec3& pos);

	/**
	 * \brief Returns the cell closest to the given pos.
	 * 
	 * \param pos - a point in space
	 * \return - the cell
	 */
	MacGridCell& getCellAt(const glm::dvec3& pos);

	/**
	 * Returns all 8 cells closest to a point in space.
	 * 
	 * \param pos - a point in space
	 * \return - an array of the 8 cells
	 */
	std::array<MacGridCellRef, 8> getCellsAround(const glm::dvec3& pos);

	/**
	 * Returns the cell given by the pos, the axis and offset.
	 *
	 * \param axis - the axis (0 - x, 1 - y, 2 - z)
	 * \param offset - the offset of the axis
	 * \param pos - the position of the cell
	 * \return - the cell
	 */
	template<int axis = 0, int offset = 0>
	inline MacGridCell& cell(const glm::ivec3& pos) {
		if constexpr (axis == 0) {
			return rawCells[(pos.x + offset) * yzMultiplier + pos.y * gridSize.z + pos.z];
		}
		else if constexpr (axis == 1) {
			return rawCells[pos.x * yzMultiplier + (pos.y + offset) * gridSize.z + pos.z];
		}
		else {
			return rawCells[pos.x * yzMultiplier + pos.y * gridSize.z + pos.z + offset];
		}
	}

	inline bool isPosValid(const glm::ivec3& pos) {
		return pos.x >= 0 && pos.x < gridSize.x && pos.y >= 0 && pos.y < gridSize.y && pos.z >= 0 && pos.z < gridSize.z;
	}

	/**
	 * Returns the cell given by the coords.
	 * 
	 * \param x
	 * \param y
	 * \param z
	 * \return the cell
	 */
	inline MacGridCell& cell(int x, int y, int z) {
		return rawCells[x * yzMultiplier + y * gridSize.z + z];
	}

	/**
	 * Run a certain lambda for each gridcell.
	 * 
	 * \param parallel - if true the function runs in parallel for each x value
	 * \param includeBorders - includes also border cells if true
	 * \param lambda - the lambda to run for each gridcell
	 */
	void forEachCell(bool parallel, bool includeBorders, std::function<void(glm::ivec3 pos, MacGridCell&)>&& lambda);

	/**
	 * Run a certain lambda for each fluid gridcell. Can only be called after postP2GUpdate.
	 * 
	 * \param parallel - if true the function runs in parallel for each x value
	 * \param lambda - the lambda to run for each gridcell
	 */
	void forEachFluidCell(bool parallel, std::function<void(glm::ivec3 pos, MacGridCell&)>&& lambda);

	/**
	 * Restores all cells to the solid state that should be solid and updates all the velocities they 
	 */
	void restoreBorderingSolidCellsAndSpeeds(bool parallel);

	/**
	 * Updates the validity of the faces based on the cell type, sets the v2 component to v plus the gravity increment, 
	 * calculates the fluid cell indeces.
	 * 
	 * \param gravityIncrement - how much to increment the vertical speed component
	 */
	void postP2GUpdate(bool parallel, double gravityIncrement);

	/**
	 * Extrapolates the velocities of the grid to walls and air cells (needed for correct advection).
 	 * 
 	 * \param parallel - if true the function runs in parallel for each x value
 	 */
	void extrapolateVelocities(bool parallel);

	/**
	 * Calculates the min and max pos of a rectangle with a given pos and size (so that the returned values align with the solid region on the grid).
	 * 
	 * \param pos - the center of the rectangle
	 * \param size - the size of the rectangle
	 * \return - a pair of the min and max pos
	 */
	inline std::pair<glm::dvec3, glm::dvec3> getMinMaxRect(const glm::dvec3& pos, const glm::dvec3& size) {
		const glm::dvec3 center = pos * cellDInv;
		const glm::dvec3 d = size * cellDInv * 0.5;
		const glm::ivec3 min(std::max(std::round(center.x - d.x), 1.0), std::max(std::round(center.y - d.y), 1.0), std::max(std::round(center.z - d.z), 1.0));
		const glm::ivec3 max(std::min(std::round(center.x + d.x), gridSize.x - 1.0), std::min(std::round(center.y + d.y), gridSize.y - 1.0), std::min(std::round(center.z + d.z), gridSize.z - 1.0));
		return std::make_pair(glm::toDvec3(min) * cellD, glm::toDvec3(max) * cellD);
	}

	/**
	 * Resets all grid values to their default (avg particle number, faces, type to AIR).
	 */
	void resetGridValues(bool parallel);

	/**
	 * Adds an obstacle to the grid (makes the cells solid and sets their face values).
	 * 
	 * \param obstacle - an obstacle
	 */
	void addObstacle(bool parallel, const genericfsim::obstacle::Obstacle* obstacle);

	/**
	 * Abstract function that solves incompressibility on the grid.
	 * 
	 * \param parallel - if true the incompressibility is solved in parallel
	 * \return - the number of iterations needed to solve the incompressibility
	 */
	virtual int solveIncompressibility(bool parallel, double dt) = 0;

	void backupGrid();

	void restoreGrid();

public:
	const glm::dvec3 cellD;
	const glm::dvec3 cellDInv;
	const glm::ivec3 gridSize;
	const glm::dvec3 dimensions;

	bool isTopOfContainerSolid = false;

	double pressureK = 2.0, averagePressure = 2.0;
	int incompressibilityMaxIterationCount = 80;
	bool pressureEnabled = true;
	double fluidDensity = 1.0;
	double residualTolerance = 1e-6;


	const bool twoD;

protected:
	const int yzMultiplier;
	const int cellCount;

	std::vector<MacGridCell> rawCells;
	std::vector<MacGridCell> rawCellsCopy;
	std::vector<glm::ivec3> fluidCellPositions;
	std::vector<glm::ivec3> fluidCellPositionsCopy;

private:
	void initNewGrid();

};


} //namespace genericfsim
