#include "bridsonSolverGrid.h"
#include <iostream>
#include <atomic>
#include <mutex>

using namespace genericfsim::macgrid;

BridsonSolverGrid::BridsonSolverGrid(const glm::dvec3& dimensions, float cellD, bool twoD, double fluidDensity = 1.0) : MacGrid(dimensions, cellD, twoD) {
	this->fluidDensity = fluidDensity;
}

void parallelFor(bool parallel, int xStart, int xEnd, std::function<void(int)>&& func) {
	if (parallel) {
#pragma omp parallel for
		for (int x = xStart; x < xEnd; x++) {
			func(x);
		}
	}
	else {
		for (int x = xStart; x < xEnd; x++) {
			func(x);
		}
	}
}

void reverseParallelFor(bool parallel, int xStart, int xEnd, std::function<void(int)>&& func) {
	if (parallel) {
#pragma omp parallel for
		for (int x = xStart; x > xEnd; x--) {
			func(x);
		}
	}
	else {
		for (int x = xStart; x > xEnd; x--) {
			func(x);
		}
	}
}

void BridsonSolverGrid::calculateAMatrix(bool parallel, double dt) {
	const double scale = dt / (fluidDensity * cellD.x * cellD.x);
	parallelFor(parallel, 0, fluidCellCount, [&](int p) {
		AMatrixRow& row = aMatrix[p];
		row.nonSolidNeighbours = 0;
		row.xWater = 0;
		row.yWater = 0;
		row.zWater = 0;
		const glm::ivec3 pos = fluidCellPositions[p];

		if (cell<0,1>(pos).type == MacGridCell::CellType::WATER) {
			row.nonSolidNeighbours += scale;
			row.xWater = -scale;
		}
		else if (cell<0,1>(pos).type == MacGridCell::CellType::AIR)
			row.nonSolidNeighbours += scale;
		if (cell<0,-1>(pos).type != MacGridCell::CellType::SOLID)
			row.nonSolidNeighbours += scale;

		if (cell<1,1>(pos).type == MacGridCell::CellType::WATER) {
			row.nonSolidNeighbours += scale;
			row.yWater = -scale;
		}
		else if (cell<1,1>(pos).type == MacGridCell::CellType::AIR)
			row.nonSolidNeighbours += scale;
		if (cell<1,-1>(pos).type != MacGridCell::CellType::SOLID)
			row.nonSolidNeighbours += scale;

		if (cell<2,1>(pos).type == MacGridCell::CellType::WATER) {
			row.nonSolidNeighbours += scale;
			row.zWater = -scale;
		}
		else if (cell<2,1>(pos).type == MacGridCell::CellType::AIR)
			row.nonSolidNeighbours += scale;
		if (cell<2,-1>(pos).type != MacGridCell::CellType::SOLID)
			row.nonSolidNeighbours += scale;
	});
}

std::vector<double> BridsonSolverGrid::calculateRHS(bool parallel) {
	std::vector<double> rhs(fluidCellCount, 0.0);
	const double scale = 1.0 / cellD.x;
	parallelFor(parallel, 0, fluidCellCount, [&](int index) {
		const glm::ivec3 pos = fluidCellPositions[index];
		const auto& currentCell = cell(pos);
		rhs[index] = -scale * (currentCell.faces[0].v2 + currentCell.faces[1].v2 + currentCell.faces[2].v2
			- cell<0,-1>(pos).faces[0].v2 - cell<1,-1>(pos).faces[1].v2 - cell<2,-1>(pos).faces[2].v2) + (pressureEnabled ? (currentCell.avgPNum - averagePressure) * pressureK : 0.0);
	});
	return rhs;
}

void BridsonSolverGrid::calculatePreconditioner() {
	preconditioner.assign(fluidCellCount, 0.0);
	for (int index = 0; index < fluidCellCount; index++) {
		const glm::ivec3 pos = fluidCellPositions[index];

		double eNeg = 0;
		double eNegTau = 0;
		if (const auto& currentCell = cell<0,-1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			const auto& Axneg = aMatrix[fluidCellId];
			const double AxnegTimesPrecon = Axneg.xWater * preconditioner[fluidCellId];
			eNeg += AxnegTimesPrecon * AxnegTimesPrecon;
			eNegTau += AxnegTimesPrecon * (Axneg.yWater + Axneg.zWater) * preconditioner[fluidCellId];
		}
		if (const auto& currentCell = cell<1,-1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			const auto& Ayneg = aMatrix[fluidCellId];
			const double AynegTimesPrecon = Ayneg.yWater * preconditioner[fluidCellId];
			eNeg += AynegTimesPrecon * AynegTimesPrecon;
			eNegTau += AynegTimesPrecon * (Ayneg.xWater + Ayneg.zWater) * preconditioner[fluidCellId];
		}
		if (const auto& currentCell = cell<2,-1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			const auto& Azneg = aMatrix[fluidCellId];
			const double AznegTimesPrecon = Azneg.zWater * preconditioner[fluidCellId];
			eNeg += AznegTimesPrecon * AznegTimesPrecon;
			eNegTau += AznegTimesPrecon * (Azneg.xWater + Azneg.yWater) * preconditioner[fluidCellId];
		}
		double e = aMatrix[index].nonSolidNeighbours - eNeg - eNegTau * tau;
		if(e < sigma * aMatrix[index].nonSolidNeighbours)
			e = (aMatrix[index].nonSolidNeighbours < 1e-6 ? 1.0 : aMatrix[index].nonSolidNeighbours);
		preconditioner[index] = 1.0 / sqrt(e);
	}
}

void BridsonSolverGrid::applyPreconditioner(const std::vector<double>& r, std::vector<double>& q_scratchpad, std::vector<double>& result) {
	const int xMult = gridSize.z * gridSize.y;
	for (int index = 0; index < fluidCellCount; index++) {
		const glm::ivec3 pos = fluidCellPositions[index];
		double qneg = 0;
		if (const auto& currentCell = cell<0,-1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			qneg += aMatrix[fluidCellId].xWater * q_scratchpad[fluidCellId] * preconditioner[fluidCellId];
		}
		if (const auto& currentCell = cell<1,-1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			qneg += aMatrix[fluidCellId].yWater * q_scratchpad[fluidCellId] * preconditioner[fluidCellId];
		}
		if (const auto& currentCell = cell<2,-1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			qneg += aMatrix[fluidCellId].zWater * q_scratchpad[fluidCellId] * preconditioner[fluidCellId];
		}
		q_scratchpad[index] = (r[index] - qneg) * preconditioner[index];
	}
	for (int index = fluidCellCount - 1; index >= 0; index--) {
		const glm::ivec3 pos = fluidCellPositions[index];
		double tneg = 0;
		const auto& currentA = aMatrix[index];
		if (const auto& currentCell = cell<0,1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			tneg += currentA.xWater * result[fluidCellId];
		}
		if (const auto& currentCell = cell<1,1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			tneg += currentA.yWater * result[fluidCellId];
		}
		if (const auto& currentCell = cell<2,1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			tneg += currentA.zWater * result[fluidCellId];
		}
		result[index] = (q_scratchpad[index] - tneg * preconditioner[index]) * preconditioner[index];
	}
}

void BridsonSolverGrid::applyAMatrix(bool parallel, const std::vector<double>& vec, std::vector<double>& result) {
	const int xMult = gridSize.z * gridSize.y;
	result.assign(fluidCellCount, 0.0);
	parallelFor(parallel, 0, fluidCellCount, [&](int index) {
		const glm::ivec3 pos = fluidCellPositions[index];
		const auto& currentA = aMatrix[index];
		double value = currentA.nonSolidNeighbours * vec[index];
		if (const auto& currentCell = cell<0,1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			value += currentA.xWater * vec[fluidCellId];
		}
		if (const auto& currentCell = cell<1,1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			value += currentA.yWater * vec[fluidCellId];
		}
		if (const auto& currentCell = cell<2,1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			value += currentA.zWater * vec[fluidCellId];
		}
		if (const auto& currentCell = cell<0,-1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			value += aMatrix[fluidCellId].xWater * vec[fluidCellId];
		}
		if (const auto& currentCell = cell<1,-1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			value += aMatrix[fluidCellId].yWater * vec[fluidCellId];
		}
		if (const auto& currentCell = cell<2,-1>(pos); currentCell.type == MacGridCell::CellType::WATER) {
			const int fluidCellId = currentCell.id;
			value += aMatrix[fluidCellId].zWater * vec[fluidCellId];
		}
		result[index] = value;
	});
}

double dotProduct(bool parallel, const std::vector<double>& vec1, const std::vector<double>& vec2) {
	double result = 0.0;
	if (parallel) {
		#pragma omp parallel for reduction(+:result)
		for (int i = 0; i < vec1.size(); i++) {
			result += vec1[i] * vec2[i];
		}
	}
	else {
		for (int i = 0; i < vec1.size(); i++) {
			result += vec1[i] * vec2[i];
		}
	}
	return result;
}

void multAdd(bool parallel, std::vector<double>& vec, const std::vector<double>& vec1, double scalar) {
	if (parallel) {
#pragma omp parallel for
		for (int i = 0; i < vec.size(); i++) {
			vec[i] += vec1[i] * scalar;
		}
	}
	else {
		for (int i = 0; i < vec.size(); i++) {
			vec[i] += vec1[i] * scalar;
		}
	}
}

void multSelfAndAdd(bool parallel, std::vector<double>& vec, const std::vector<double>& vec1, double scalar) {
	if (parallel) {
#pragma omp parallel for
		for (int i = 0; i < vec.size(); i++) {
			vec[i] = vec[i] * scalar + vec1[i];
		}
	}
	else {
		for (int i = 0; i < vec.size(); i++) {
			vec[i] = vec[i] * scalar + vec1[i];
		}
	}
}

int BridsonSolverGrid::solveIncompressibility(bool parallel, double dt) {
	fluidCellCount = fluidCellPositions.size();
	aMatrix.resize(fluidCellCount);
	preconditioner.resize(fluidCellCount);

	std::vector<double> pressure(fluidCellCount, 0.0);
	std::vector<double> z(fluidCellCount, 0.0);
	std::vector<double> q_scratchpad(fluidCellCount, 0.0);
	std::vector<double> r = calculateRHS(parallel);

	double total = 0;
	for (auto& d : r)
		total += d * d;
	if (total < 1e-7)
		return 0;

	calculateAMatrix(parallel, dt);
	calculatePreconditioner();
	applyPreconditioner(r, q_scratchpad, z);
	std::vector<double> s = z;

	double sigma = dotProduct(parallel, z, r);
	int it = 0;
	for (; it < incompressibilityMaxIterationCount; it++) {
		applyAMatrix(parallel, s, z);
		
		double alpha = sigma / dotProduct(parallel, s, z);
		if(alpha != alpha)
			break;
		multAdd(parallel, pressure, s, alpha);
		multAdd(parallel, r, z, -alpha);

		double max = 0.0;
		for(auto& d : r)
			if (std::abs(d) > max)
				max = std::abs(d);
		if (max < residualTolerance)
			break;

		applyPreconditioner(r, q_scratchpad, z);
		double sigmaNew = dotProduct(parallel, z, r);
		if(sigma != sigma)
			break;
		double beta = sigmaNew / sigma;
		multSelfAndAdd(parallel, s, z, beta);
		sigma = sigmaNew;
	}
	applyPressureToVelocities(parallel, dt, pressure);
	return it;
}

void BridsonSolverGrid::applyPressureToVelocities(bool parallel, double dt, const std::vector<double>& pressures) {
	const double scale = dt / (fluidDensity * cellD.x);
	parallelFor(parallel, 0, fluidCellCount, [&](int index) {
		const glm::ivec3 pos = fluidCellPositions[index];
		auto& currentCell = cell(pos);
		
		if (const auto& cellx = cell<0, 1>(pos); cellx.type != MacGridCell::CellType::SOLID) {
			if (cellx.type == MacGridCell::CellType::AIR)
				currentCell.faces[0].v2 += scale * pressures[index];
			else
				currentCell.faces[0].v2 += scale * (pressures[index] - pressures[cellx.id]);
		}
		if (const auto& celly = cell<1, 1>(pos); celly.type != MacGridCell::CellType::SOLID) {
			if (celly.type == MacGridCell::CellType::AIR)
				currentCell.faces[1].v2 += scale * pressures[index];
			else
				currentCell.faces[1].v2 += scale * (pressures[index] - pressures[celly.id]);
		}
		if (const auto& cellz = cell<2, 1>(pos); cellz.type != MacGridCell::CellType::SOLID) {
			if (cellz.type == MacGridCell::CellType::AIR)
				currentCell.faces[2].v2 += scale * pressures[index];
			else
				currentCell.faces[2].v2 += scale * (pressures[index] - pressures[cellz.id]);
		}

		if (auto& cellx = cell<0,-1>(pos); cellx.type == MacGridCell::CellType::AIR)
			cellx.faces[0].v2 -= scale * pressures[index];
		if (auto& celly = cell<1,-1>(pos); celly.type == MacGridCell::CellType::AIR)
			celly.faces[1].v2 -= scale * pressures[index];
		if (auto& cellz = cell<2,-1>(pos); cellz.type == MacGridCell::CellType::AIR)
			cellz.faces[2].v2 -= scale * pressures[index];
	});
}

std::shared_ptr<MacGrid> BridsonSolverGrid::clone() const
{
	return std::make_shared<BridsonSolverGrid>(*this);
}

