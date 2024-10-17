#pragma once

#include "macGrid.h"

namespace genericfsim::macgrid {

class BasicMacGrid : public MacGrid {
public:
	using MacGrid::MacGrid;

	int solveIncompressibility(bool parallel, double dt) override;

	std::shared_ptr<MacGrid> clone() const override;

private:
	void solveIncompressibilityParallel();
	void solveIncompressibilitySingleThreaded();
};

}
