#include "macGrid.h"
#include <iostream>
#include "../util/compTimeForLoop.h"

using namespace genericfsim::macgrid;
using namespace genericfsim::obstacle;


MacGrid::MacGrid(glm::dvec3 targetDimensions, double resolution, bool twoD) 
	: cellD(1 / resolution, 1 / resolution, twoD ? targetDimensions.z / 3 : 1 / resolution), cellDInv(1.0 / cellD),
	gridSize(targetDimensions.x / cellD.x, targetDimensions.y / cellD.y, twoD ? 3 : targetDimensions.z / cellD.z),
	dimensions(gridSize.x * cellD.x, gridSize.y * cellD.y, twoD ? targetDimensions.z : gridSize.z * cellD.z), twoD(twoD), 
	yzMultiplier(gridSize.y * gridSize.z), cellCount(gridSize.x * gridSize.y * gridSize.z) {

	initNewGrid();
	restoreBorderingSolidCellsAndSpeeds(true);
}


void MacGrid::initNewGrid() {
	rawCells.clear();
	for (int x = 0; x < gridSize.x; x++) {
		for (int y = 0; y < gridSize.y; y++) {
			for (int z = 0; z < gridSize.z; z++) {
				MacGridCell cell{ glm::dvec3((x + 1) * cellD.x, (y + 0.5) * cellD.y, (z + 0.5) * cellD.z),
									glm::dvec3((x + 0.5) * cellD.x, (y + 1) * cellD.y, (z + 0.5) * cellD.z),
									glm::dvec3((x + 0.5) * cellD.x, (y + 0.5) * cellD.y, (z + 1) * cellD.z),
									glm::dvec3(x + 0.5, y + 0.5, z + 0.5) * cellD };
				rawCells.push_back(cell);
			}
		}
	}
}


void MacGrid::forEachCell(bool parallel, bool includeBorders, std::function<void(glm::ivec3 pos, MacGridCell&)>&& lambda) {
	int b = !includeBorders;
	if (parallel) {
#pragma omp parallel for
		for (int x = b; x < gridSize.x - b; x++) {
			for (int y = b; y < gridSize.y - b; y++) {
				for (int z = b; z < gridSize.z - b; z++) {
					lambda(glm::ivec3(x, y, z), cell(x, y, z));
				}
			}
		}
	}
	else {
		for (int x = b; x < gridSize.x - b; x++) {
			for (int y = b; y < gridSize.y - b; y++) {
				for (int z = b; z < gridSize.z - b; z++) {
					lambda(glm::ivec3(x, y, z), cell(x, y, z));
				}
			}
		}
	}
}

void MacGrid::forEachFluidCell(bool parallel, std::function<void(glm::ivec3 pos, MacGridCell&)>&& lambda) {
	if (parallel) {
#pragma omp parallel for
		for (int p = 0; p < fluidCellPositions.size(); p++) {
			const auto& pos = fluidCellPositions[p];
			lambda(pos, cell(pos));
		}
	}
	else {
		for (int p = 0; p < fluidCellPositions.size(); p++) {
			const auto& pos = fluidCellPositions[p];
			lambda(pos, cell(pos));
		}
	}
}

void MacGrid::restoreBorderingSolidCellsAndSpeeds(bool parallel) {
	if (parallel) {
#pragma omp parallel for
		for (int y = 0; y < gridSize.y; y++) {
			for (int z = 0; z < gridSize.z; z++) {
				cell(0, y, z).type = cell(gridSize.x - 1, y, z).type = MacGridCell::CellType::SOLID;
				if(cell(1, y, z).type == MacGridCell::CellType::WATER)
					cell(0, y, z).faces[0].v = 0;
				if (cell(gridSize.x - 2, y, z).type == MacGridCell::CellType::WATER)
					cell(gridSize.x - 2, y, z).faces[0].v = 0;
			}
		}
#pragma omp parallel for
		for (int x = 0; x < gridSize.x; x++) {
			for (int z = 0; z < gridSize.z; z++) {
				cell(x, 0, z).type = MacGridCell::CellType::SOLID;
				if(cell(x, 1, z).type == MacGridCell::CellType::WATER)
					cell(x, 0, z).faces[1].v = 0;
				if (isTopOfContainerSolid) {
					cell(x, gridSize.y - 1, z).type = MacGridCell::CellType::SOLID;
					if(cell(x, gridSize.y - 2, z).type == MacGridCell::CellType::WATER)
						cell(x, gridSize.y - 2, z).faces[1].v = 0;
				}
			}
			for (int y = 0; y < gridSize.y; y++) {
				cell(x, y, 0).type = cell(x, y, gridSize.z - 1).type = MacGridCell::CellType::SOLID;
				if(cell(x, y, 1).type == MacGridCell::CellType::WATER)
					cell(x, y, 0).faces[2].v = 0;
				if (cell(x, y, gridSize.z - 2).type == MacGridCell::CellType::WATER)
					cell(x, y, gridSize.z - 2).faces[2].v = 0;
			}
		}
	}
	else {
		for (int x = 0; x < gridSize.x; x++) {
			for (int y = 0; y < gridSize.y; y++) {
				cell(x, y, 0).type = cell(x, y, gridSize.z - 1).type = MacGridCell::CellType::SOLID;
				if (cell(x, y, 1).type == MacGridCell::CellType::WATER)
					cell(x, y, 0).faces[2].v = 0;
				if (cell(x, y, gridSize.z - 2).type == MacGridCell::CellType::WATER)
					cell(x, y, gridSize.z - 2).faces[2].v = 0;
			}
		}
		for (int y = 0; y < gridSize.y; y++) {
			for (int z = 0; z < gridSize.z; z++) {
				cell(0, y, z).type = cell(gridSize.x - 1, y, z).type = MacGridCell::CellType::SOLID;
				if (cell(1, y, z).type == MacGridCell::CellType::WATER)
					cell(0, y, z).faces[0].v = 0;
				if (cell(gridSize.x - 2, y, z).type == MacGridCell::CellType::WATER)
					cell(gridSize.x - 2, y, z).faces[0].v = 0;
			}
		}
		for (int x = 0; x < gridSize.x; x++) {
			for (int z = 0; z < gridSize.z; z++) {
				cell(x, 0, z).type = MacGridCell::CellType::SOLID;
				if (cell(x, 1, z).type == MacGridCell::CellType::WATER)
					cell(x, 0, z).faces[1].v = 0;
				if (isTopOfContainerSolid) {
					cell(x, gridSize.y - 1, z).type = MacGridCell::CellType::SOLID;
					if (cell(x, gridSize.y - 2, z).type == MacGridCell::CellType::WATER)
						cell(x, gridSize.y - 2, z).faces[1].v = 0;
				}
			}
		}
	}
}

std::array<std::array<MacGridCell::FaceRef, 8>, 3> MacGrid::getFacesAround(const glm::dvec3& pos) {
	constexpr glm::dvec3 axisOffset[3] = { glm::dvec3(0.0, 0.5, 0.5), glm::dvec3(0.5, 0.0, 0.5), glm::dvec3(0.5, 0.5, 0.0) };
	glm::dvec3 gridPos[3] = { pos * cellDInv - axisOffset[0], pos * cellDInv - axisOffset[1], pos * cellDInv - axisOffset[2] };
	glm::ivec3 coord[3] = { glm::ivec3(gridPos[0].x, gridPos[0].y, gridPos[0].z), glm::ivec3(gridPos[1].x, gridPos[1].y, gridPos[1].z), glm::ivec3(gridPos[2].x, gridPos[2].y, gridPos[2].z) };
	coord[0].x -= 1;
	coord[1].y -= 1;
	coord[2].z -= 1;

	return { std::array<MacGridCell::FaceRef, 8>{
				MacGridCell::FaceRef { cell(coord[0].x + 1, coord[0].y + 1, coord[0].z + 1).faces[0]},
				MacGridCell::FaceRef { cell(coord[0].x, coord[0].y + 1, coord[0].z + 1).faces[0] },
				MacGridCell::FaceRef { cell(coord[0].x + 1, coord[0].y, coord[0].z + 1).faces[0] },
				MacGridCell::FaceRef { cell(coord[0].x + 1, coord[0].y + 1, coord[0].z).faces[0] },
				MacGridCell::FaceRef { cell(coord[0].x + 1, coord[0].y, coord[0].z).faces[0] },
				MacGridCell::FaceRef { cell(coord[0].x, coord[0].y, coord[0].z + 1).faces[0] },
				MacGridCell::FaceRef { cell(coord[0].x, coord[0].y + 1, coord[0].z).faces[0] },
				MacGridCell::FaceRef { cell(coord[0].x, coord[0].y, coord[0].z).faces[0] }
			},
			std::array<MacGridCell::FaceRef, 8>{
				MacGridCell::FaceRef { cell(coord[1].x + 1, coord[1].y + 1, coord[1].z + 1).faces[1]},
				MacGridCell::FaceRef { cell(coord[1].x, coord[1].y + 1, coord[1].z + 1).faces[1] },
				MacGridCell::FaceRef { cell(coord[1].x + 1, coord[1].y, coord[1].z + 1).faces[1] },
				MacGridCell::FaceRef { cell(coord[1].x + 1, coord[1].y + 1, coord[1].z).faces[1] },
				MacGridCell::FaceRef { cell(coord[1].x + 1, coord[1].y, coord[1].z).faces[1] },
				MacGridCell::FaceRef { cell(coord[1].x, coord[1].y, coord[1].z + 1).faces[1] },
				MacGridCell::FaceRef { cell(coord[1].x, coord[1].y + 1, coord[1].z).faces[1] },
				MacGridCell::FaceRef { cell(coord[1].x, coord[1].y, coord[1].z).faces[1] }
			},
			std::array<MacGridCell::FaceRef, 8>{
				MacGridCell::FaceRef { cell(coord[2].x + 1, coord[2].y + 1, coord[2].z + 1).faces[2]},
				MacGridCell::FaceRef { cell(coord[2].x, coord[2].y + 1, coord[2].z + 1).faces[2] },
				MacGridCell::FaceRef { cell(coord[2].x + 1, coord[2].y, coord[2].z + 1).faces[2] },
				MacGridCell::FaceRef { cell(coord[2].x + 1, coord[2].y + 1, coord[2].z).faces[2] },
				MacGridCell::FaceRef { cell(coord[2].x + 1, coord[2].y, coord[2].z).faces[2] },
				MacGridCell::FaceRef { cell(coord[2].x, coord[2].y, coord[2].z + 1).faces[2] },
				MacGridCell::FaceRef { cell(coord[2].x, coord[2].y + 1, coord[2].z).faces[2] },
				MacGridCell::FaceRef { cell(coord[2].x, coord[2].y, coord[2].z).faces[2] }
			} };

}

std::array<MacGridCellRef, 8> MacGrid::getCellsAround(const glm::dvec3& pos) {
	glm::dvec3 gridPos = pos * cellDInv - glm::dvec3(0.5, 0.5, 0.5);
	glm::ivec3 coord = glm::ivec3(gridPos.x, gridPos.y, gridPos.z);

	return {
		MacGridCellRef { cell(coord.x + 1, coord.y + 1, coord.z + 1) },
		MacGridCellRef { cell(coord.x, coord.y + 1, coord.z + 1) },
		MacGridCellRef { cell(coord.x + 1, coord.y, coord.z + 1) },
		MacGridCellRef { cell(coord.x + 1, coord.y + 1, coord.z) },
		MacGridCellRef { cell(coord.x + 1, coord.y, coord.z) },
		MacGridCellRef { cell(coord.x, coord.y, coord.z + 1) },
		MacGridCellRef { cell(coord.x, coord.y + 1, coord.z) },
		MacGridCellRef { cell(coord.x, coord.y, coord.z) }
	};
}

void MacGrid::postP2GUpdate(bool parallel, double gravityIncrement) {
	forEachCell(parallel, true, [&](glm::ivec3 pos, MacGridCell& c) {
		c.faces[0].v2 = c.faces[0].v;
		c.faces[1].v2 = c.faces[1].v;
		c.faces[2].v2 = c.faces[2].v;
		if(c.type != MacGridCell::CellType::SOLID && cell<1, 1>(pos).type != MacGridCell::CellType::SOLID)
			c.faces[1].v2 += gravityIncrement;
	});
	fluidCellPositions.clear();
	forEachCell(false, false, [&](glm::ivec3 pos, MacGridCell& c) {
		if (c.type == MacGridCell::CellType::WATER) {
			fluidCellPositions.push_back(pos);
			c.id = fluidCellPositions.size() - 1;
		}
	});
}

void MacGrid::resetGridValues(bool parallel) {
	forEachCell(parallel, true, [this](glm::ivec3, MacGridCell& cell) {
		cell.faces[0].v = cell.faces[1].v = cell.faces[2].v = 0.0;
		cell.faces[0].v2 = cell.faces[1].v2 = cell.faces[2].v2 = 0.0;
		cell.faces[0].particleWeightSum = cell.faces[1].particleWeightSum = cell.faces[2].particleWeightSum = 0.0;
		cell.avgPNum = 0.0;
		cell.type = MacGridCell::CellType::AIR;
	});
}

void MacGrid::addObstacle(bool parallel, const Obstacle* obstacle) {
	const glm::dvec3 center = obstacle->pos * cellDInv;
	const glm::dvec3 speed = obstacle->speed;

	if (const SphericalObstacle* tmp = dynamic_cast<const SphericalObstacle*>(obstacle); tmp != nullptr && dynamic_cast<const SphericalParticleSink*>(tmp) == nullptr) {
		const SphericalObstacle& sphere = *tmp;
		const double r = cellDInv.x * sphere.r;
		const double r2 = r * r;
		const glm::ivec3 min(std::max(center.x - r, 1.0), std::max(center.y - r, 1.0), std::max(center.z - r, 1.0));
		const glm::ivec3 max(std::min(center.x + r, gridSize.x - 2.0), std::min(center.y + r, gridSize.y - 2.0), std::min(center.z + r, gridSize.z - 2.0));
		for (int x = min.x; x <= max.x; x++) {
			for (int y = min.y; y <= max.y; y++) {
				for (int z = min.z; z <= max.z; z++) {
					glm::dvec3 c2o = glm::dvec3(x + 0.5, y + 0.5, z + 0.5) - center;
					if (glm::dot(c2o, c2o) < r2) {
						glm::ivec3 pos(x, y, z);
						cell(pos).type = MacGridCell::CellType::SOLID;
						if(cell<0,1>(pos).type == MacGridCell::CellType::WATER)
							cell(pos).faces[0].v = speed.x;
						if(cell<0,-1>(pos).type == MacGridCell::CellType::WATER)
							cell<0,-1>(pos).faces[0].v = speed.x;
						if(cell<1,1>(pos).type == MacGridCell::CellType::WATER)
							cell(pos).faces[1].v = speed.y;
						if(cell<1,-1>(pos).type == MacGridCell::CellType::WATER)
							cell<1,-1>(pos).faces[1].v = speed.y;
						if(cell<2,1>(pos).type == MacGridCell::CellType::WATER)
							cell(pos).faces[2].v = speed.z;
						if(cell<2,-1>(pos).type == MacGridCell::CellType::WATER)
							cell<2,-1>(pos).faces[2].v = speed.z;
					}
				}
			}
		}
	}

	if (const RectengularObstacle* tmp = dynamic_cast<const RectengularObstacle*>(obstacle); tmp != nullptr) {
		const RectengularObstacle& rect = *tmp;
		const glm::dvec3 d = rect.size * cellDInv * 0.5;
		const glm::ivec3 min(std::max(std::round(center.x - d.x), 1.0), std::max(std::round(center.y - d.y), 1.0), std::max(std::round(center.z - d.z), 1.0));
		const glm::ivec3 max(std::min(std::round(center.x + d.x), gridSize.x - 1.0), std::min(std::round(center.y + d.y), gridSize.y - 1.0), std::min(std::round(center.z + d.z), gridSize.z - 1.0));
		for (int x = min.x; x < max.x; x++) {
			for (int y = min.y; y < max.y; y++) {
				for (int z = min.z; z < max.z; z++) {
					glm::ivec3 pos(x, y, z);
					cell(pos).type = MacGridCell::CellType::SOLID;
					if (cell<0, 1>(pos).type == MacGridCell::CellType::WATER)
						cell(pos).faces[0].v = speed.x;
					if (cell<0, -1>(pos).type == MacGridCell::CellType::WATER)
						cell<0, -1>(pos).faces[0].v = speed.x;
					if (cell<1, 1>(pos).type == MacGridCell::CellType::WATER)
						cell(pos).faces[1].v = speed.y;
					if (cell<1, -1>(pos).type == MacGridCell::CellType::WATER)
						cell<1, -1>(pos).faces[1].v = speed.y;
					if (cell<2, 1>(pos).type == MacGridCell::CellType::WATER)
						cell(pos).faces[2].v = speed.z;
					if (cell<2, -1>(pos).type == MacGridCell::CellType::WATER)
						cell<2, -1>(pos).faces[2].v = speed.z;
				}
			}
		}
	}
}

void MacGrid::extrapolateVelocities(bool parallel) {
	constexpr int iterationNum = 2;
	std::vector<unsigned char> valid(cellCount, 100);
	const auto isValid = [&](const glm::ivec3& pos, int currentVal) {
		return pos.x >= 0 && pos.x < gridSize.x && pos.y >= 0 && pos.y < gridSize.y && pos.z >= 0 && pos.z < gridSize.z && valid[pos.x * yzMultiplier + pos.y * gridSize.z + pos.z] <= currentVal;
	};
	const auto setValid = [&](const glm::ivec3& pos, int currentVal) {
		valid[pos.x * yzMultiplier + pos.y * gridSize.z + pos.z] = currentVal;
	};
	forEachFluidCell(true, [&](glm::ivec3 pos, MacGridCell& cell) {
		setValid(pos, 0);
	});

	for (int it = 0; it < iterationNum; it++) {
		forEachCell(parallel, true, [&](glm::ivec3 pos, MacGridCell& c) {
			if (isValid(pos, it))
				return;
			int validNeighbourCount = 0;
			glm::dvec3 vSum(0.0, 0.0, 0.0);
			COMP_FOR_LOOP(axis, 3, {
				COMP_FOR_LOOP(offset, 2, {
					glm::ivec3 newPos = pos;
					newPos[axis] += offset * 2 - 1;
					if (isValid(newPos, it)) {
						vSum[0] += (cell(newPos).faces[0].v2);
						vSum[1] += (cell(newPos).faces[1].v2);
						vSum[2] += (cell(newPos).faces[2].v2);
						validNeighbourCount++;
					}
				});
			});
			if (validNeighbourCount > 0) {
				if(isPosValid(pos + glm::ivec3(1, 0, 0)) && cell<0, 1>(pos).type != MacGridCell::CellType::WATER)
					c.faces[0].v2 = vSum[0] / validNeighbourCount;
				if (isPosValid(pos + glm::ivec3(0, 1, 0)) && cell<1, 1>(pos).type != MacGridCell::CellType::WATER)
					c.faces[1].v2 = vSum[1] / validNeighbourCount;
				if (isPosValid(pos + glm::ivec3(0, 0, 1)) && cell<2, 1>(pos).type != MacGridCell::CellType::WATER)
					c.faces[2].v2 = vSum[2] / validNeighbourCount;
				setValid(pos, it + 1);
			}
		});
	}
}
