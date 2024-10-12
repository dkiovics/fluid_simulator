#pragma once

#include <compute/computeProgram.h>
#include <compute/storageBuffer.h>
#include <param.hpp>
#include "gfx3D/optimizer/adam.h"
#include "gfx3D/renderer3DInterface.h"

namespace visual
{

class DensityControl : public ParamLineCollection
{
public:
	struct AvgMovement
	{
		float avgMovement = 0.0f;
		int index = 0;
	};

	DensityControl();

	DensityControl(const DensityControl& other) = delete;
	DensityControl(DensityControl&& other) noexcept = delete;
	DensityControl& operator=(const DensityControl& other) = delete;
	DensityControl& operator=(DensityControl&& other) noexcept = delete;

	void setParamNum(size_t paramNum);

	void reset();

	bool updateAvgMovement(renderer::ssbo_ptr<float> data);

	renderer::ssbo_ptr<AvgMovement> getAvgMovementData();

	void updatePositions(renderer::ssbo_ptr<visual::ParticleShaderData> data);

private:
	ParamInt sampleNum = ParamInt("Sample size", 40, 1, 200);
	ParamFloat rollingAvgAlpha = ParamFloat("Rolling avg alpha", 0.1f, 0.01f, 1.0f, "%.2f");
	ParamInt particlePercantageToMove = ParamInt("Particle percantage to move", 12, 1, 40);
	ParamFloat particleSpread = ParamFloat("New particle spread", 0.2f, 0.05f, 1.0f, "%.2f");

	int sampleCount = 0;

	renderer::compute_ptr avgMovementCompute;
	renderer::compute_ptr initAvgMovementArray;
	renderer::compute_ptr updatePositionsCompute;

	renderer::ssbo_ptr<AvgMovement> avgMovement;
	
};


} // namespace visual
