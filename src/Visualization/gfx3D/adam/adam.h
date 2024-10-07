#pragma once

#include <param.hpp>
#include <compute/computeProgram.h>
#include <compute/storageBuffer.h>
#include "gfx3D/renderer3DInterface.h"

namespace visual
{

class AdamOptimizer : public ParamLineCollection
{
public:
	AdamOptimizer(size_t paramNum);

	AdamOptimizer(const AdamOptimizer& other) = delete;
	AdamOptimizer(AdamOptimizer&& other) noexcept = delete;
	AdamOptimizer& operator=(const AdamOptimizer& other) = delete;
	AdamOptimizer& operator=(AdamOptimizer&& other) noexcept = delete;

	void setParamNum(size_t paramNum);

	void reset();

	void set(const renderer::ssbo_ptr<float> data);

	bool updateGradient(const renderer::ssbo_ptr<float> g);

	renderer::ssbo_ptr<float> getOptimizedFloatData() const;

private:
	ParamInt gradientSampleNum = ParamInt("Gradient sample size", 10, 1, 100);
	ParamFloat alpha = ParamFloat("Alpha", 0.004f, 0.001f, 0.5f, "%.3f");
	ParamFloat beta1 = ParamFloat("Beta1", 0.9f, 0.1f, 0.99f);
	ParamFloat beta2 = ParamFloat("Beta2", 0.999f, 0.9f, 0.9999f, "%.4f");
	ParamFloat epsilon = ParamFloat("Epsilon", 1e-8f, 1e-10f, 1e-6f, "%e");

	int gradientSampleCount = 0;
	float t = 0.0f;

	renderer::ssbo_ptr<float> gradientSum;
	renderer::ssbo_ptr<float> optimizedData;
	renderer::ssbo_ptr<float> mVec;
	renderer::ssbo_ptr<float> vVec;

	renderer::compute_ptr gradientSumCompute;
	renderer::compute_ptr adamOptimizerCompute;

};

} // namespace visual