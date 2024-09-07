#include "basicMacGrid.h"

using namespace genericfsim::macgrid;

int BasicMacGrid::solveIncompressibility(bool parallel, double dt) {
	if (parallel)
		solveIncompressibilityParallel();
	else
		solveIncompressibilitySingleThreaded();
	return incompressibilityMaxIterationCount;
}

constexpr double overRelaxation = 1.98;

void BasicMacGrid::solveIncompressibilityParallel() {
	for (int n = 0; n < incompressibilityMaxIterationCount; n++) {
#pragma omp parallel for
		for (int x = 1; x < gridSize.x - 1; x++) {
			for (int y = 1; y < gridSize.y - 1; y += 1) {
				for (int z = 1 + ((x + y) % 2); z < gridSize.z - 1; z += 2) {
					glm::ivec3 pos(x, y, z);
					auto& currentCell = cell(pos);
					if (currentCell.type != MacGridCell::CellType::WATER)
						continue;
					const auto& cellXpos = cell<0, 1>(pos);
					auto& cellXneg = cell<0, -1>(pos);
					const auto& cellYpos = cell<1, 1>(pos);
					auto& cellYneg = cell<1, -1>(pos);
					const auto& cellZpos = cell<2, 1>(pos);
					auto& cellZneg = cell<2, -1>(pos);
					int s1 = cellZpos.type != MacGridCell::CellType::SOLID;
					int s2 = cellZneg.type != MacGridCell::CellType::SOLID;
					int s3 = cellYpos.type != MacGridCell::CellType::SOLID;
					int s4 = cellYneg.type != MacGridCell::CellType::SOLID;
					int s5 = cellXpos.type != MacGridCell::CellType::SOLID;
					int s6 = cellXneg.type != MacGridCell::CellType::SOLID;
					int s = s1 + s2 + s3 + s4 + s5 + s6;
					if (s == 0)
						continue;
					double d = -currentCell.faces[0].v2 - currentCell.faces[1].v2 - currentCell.faces[2].v2
						+ cellXneg.faces[0].v2 + cellYneg.faces[1].v2 + cellZneg.faces[2].v2
						+ (pressureEnabled ? (currentCell.avgPNum - averagePressure) * pressureK : 0.0);
					d = d * overRelaxation / s;
					if (s1)
						currentCell.faces[2].v2 += d;
					if (s2)
						cellZneg.faces[2].v2 -= d;
					if (s3)
						currentCell.faces[1].v2 += d;
					if (s4)
						cellYneg.faces[1].v2 -= d;
					if (s5)
						currentCell.faces[0].v2 += d;
					if (s6)
						cellXneg.faces[0].v2 -= d;
				}
			}
		}
#pragma omp parallel for
		for (int x = 1; x < gridSize.x - 1; x++) {
			for (int y = 1; y < gridSize.y - 1; y += 1) {
				for (int z = 2 - ((x + y) % 2); z < gridSize.z - 1; z += 2) {
					glm::ivec3 pos(x, y, z);
					auto& currentCell = cell(pos);
					if (currentCell.type != MacGridCell::CellType::WATER)
						continue;
					const auto& cellXpos = cell<0, 1>(pos);
					auto& cellXneg = cell<0, -1>(pos);
					const auto& cellYpos = cell<1, 1>(pos);
					auto& cellYneg = cell<1, -1>(pos);
					const auto& cellZpos = cell<2, 1>(pos);
					auto& cellZneg = cell<2, -1>(pos);
					int s1 = cellZpos.type != MacGridCell::CellType::SOLID;
					int s2 = cellZneg.type != MacGridCell::CellType::SOLID;
					int s3 = cellYpos.type != MacGridCell::CellType::SOLID;
					int s4 = cellYneg.type != MacGridCell::CellType::SOLID;
					int s5 = cellXpos.type != MacGridCell::CellType::SOLID;
					int s6 = cellXneg.type != MacGridCell::CellType::SOLID;
					int s = s1 + s2 + s3 + s4 + s5 + s6;
					if (s == 0)
						continue;
					double d = -currentCell.faces[0].v2 - currentCell.faces[1].v2 - currentCell.faces[2].v2
						+ cellXneg.faces[0].v2 + cellYneg.faces[1].v2 + cellZneg.faces[2].v2
						+ (pressureEnabled ? (currentCell.avgPNum - averagePressure) * pressureK : 0.0);
					d = d * overRelaxation / s;
					if (s1)
						currentCell.faces[2].v2 += d;
					if (s2)
						cellZneg.faces[2].v2 -= d;
					if (s3)
						currentCell.faces[1].v2 += d;
					if (s4)
						cellYneg.faces[1].v2 -= d;
					if (s5)
						currentCell.faces[0].v2 += d;
					if (s6)
						cellXneg.faces[0].v2 -= d;
				}
			}
		}
	}
}

void BasicMacGrid::solveIncompressibilitySingleThreaded() {
	for (int n = 0; n < incompressibilityMaxIterationCount; n++) {
		for (int x = 1; x < gridSize.x - 1; x++) {
			for (int y = 1; y < gridSize.y - 1; y++) {
				for (int z = 1; z < gridSize.z - 1; z++) {
					glm::ivec3 pos(x, y, z);
					auto& currentCell = cell(pos);
					if (currentCell.type != MacGridCell::CellType::WATER)
						continue;
					const auto& cellXpos = cell<0, 1>(pos);
					auto& cellXneg = cell<0, -1>(pos);
					const auto& cellYpos = cell<1, 1>(pos);
					auto& cellYneg = cell<1, -1>(pos);
					const auto& cellZpos = cell<2, 1>(pos);
					auto& cellZneg = cell<2, -1>(pos);
					int s1 = cellZpos.type != MacGridCell::CellType::SOLID;
					int s2 = cellZneg.type != MacGridCell::CellType::SOLID;
					int s3 = cellYpos.type != MacGridCell::CellType::SOLID;
					int s4 = cellYneg.type != MacGridCell::CellType::SOLID;
					int s5 = cellXpos.type != MacGridCell::CellType::SOLID;
					int s6 = cellXneg.type != MacGridCell::CellType::SOLID;
					int s = s1 + s2 + s3 + s4 + s5 + s6;
					if (s == 0)
						continue;
					double d = -currentCell.faces[0].v2 - currentCell.faces[1].v2 - currentCell.faces[2].v2
						+ cellXneg.faces[0].v2 + cellYneg.faces[1].v2 + cellZneg.faces[2].v2
						+ (pressureEnabled ? (currentCell.avgPNum - averagePressure) * pressureK : 0.0);
					d = d * overRelaxation / s;
					if (s1)
						currentCell.faces[2].v2 += d;
					if (s2)
						cellZneg.faces[2].v2 -= d;
					if (s3)
						currentCell.faces[1].v2 += d;
					if (s4)
						cellYneg.faces[1].v2 -= d;
					if (s5)
						currentCell.faces[0].v2 += d;
					if (s6)
						cellXneg.faces[0].v2 -= d;
				}
			}
		}
	}
}
