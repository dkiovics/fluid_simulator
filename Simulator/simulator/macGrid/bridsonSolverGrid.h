#pragma once

#include <glm/glm.hpp>
#include "macGrid.h"
#include "macGridCell.h"
#include <vector>



namespace genericfsim::macgrid {

class BridsonSolverGrid : public MacGrid {
public:
	BridsonSolverGrid(const glm::dvec3& dimensions, float cellD, bool twoD, double fluidDensity);
	
	int solveIncompressibility(bool parallel, double dt) override;

private:
	struct AMatrixRow {
		double nonSolidNeighbours;
		double xWater;
		double yWater;
		double zWater;
	};

	constexpr static double tau = 0.97;
	constexpr static double sigma = 0.25;

	std::vector<AMatrixRow> aMatrix;
	std::vector<double> preconditioner;
	int fluidCellCount = 0;

	void calculateAMatrix(bool parallel, double dt);
	std::vector<double> calculateRHS(bool parallel);
	void calculatePreconditioner();
	
	void applyPreconditioner(const std::vector<double>& r, std::vector<double>& q_scratchpad, std::vector<double>& result);
	void applyAMatrix(bool parallel, const std::vector<double>& vec, std::vector<double>& result);

	void applyPressureToVelocities(bool parallel, double dt, const std::vector<double>& pressure);
};

}
