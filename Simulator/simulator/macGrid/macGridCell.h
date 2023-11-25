#pragma once

#include <atomic>
#include <glm/glm.hpp>

namespace genericfsim::macgrid {

struct MacGridCell {
	struct Face {
		std::atomic<double> v = 0.0;
		double v2 = 0;
		std::atomic<double> particleWeightSum = 0.0;
		const glm::dvec3 pos;

		Face(glm::dvec3 pos) : pos(std::move(pos)) { }
		Face(const Face& face) : pos(face.pos) { }
		const Face& operator=(const Face& face) {
			v = face.v.load();
			v2 = face.v2;
			particleWeightSum = face.particleWeightSum.load();
			return *this;
		}
	};

	struct FaceRef {
		Face& face;
	};

	enum class CellType {
		WATER, AIR, SOLID
	};

	Face faces[3];   //x, y, z faces
	const glm::dvec3 pos;
	CellType type = CellType::AIR;
	std::atomic<double> avgPNum = 0.0;
	int id = 0;

	MacGridCell(const Face& fx, const Face& fy, const Face& fz, const glm::dvec3& pos)
		: faces{ fx, fy, fz }, pos(pos) { }

	MacGridCell(const MacGridCell& cell)
		: faces{ cell.faces[0], cell.faces[1], cell.faces[2] }, pos(cell.pos),
		type(cell.type), avgPNum(cell.avgPNum.load()) { }

	const MacGridCell& operator=(const MacGridCell& cell) {
		faces[0] = cell.faces[0];
		faces[1] = cell.faces[1];
		faces[2] = cell.faces[2];
		type = cell.type;
		avgPNum = cell.avgPNum.load();
		return *this;
	}
};

struct MacGridCellRef {
	MacGridCell& cell;
};

}
