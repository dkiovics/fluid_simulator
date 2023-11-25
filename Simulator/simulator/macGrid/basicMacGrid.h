#pragma once

#include "macGrid.h"

namespace genericfsim::macgrid {

class BasicMacGrid : public MacGrid {
public:
	using MacGrid::MacGrid;

	int solveIncompressibility(bool parallel, double dt) override;

private:
	void solveIncompressibilityParallel();
	void solveIncompressibilitySingleThreaded();
};

}
