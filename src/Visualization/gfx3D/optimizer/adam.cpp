#include "adam.h"

using namespace visual;


visual::AdamOptimizer::AdamOptimizer(size_t paramNum)
{
	setParamNum(paramNum);
	adamOptimizerCompute = renderer::make_compute("shaders/3D/adam/adamOptimizer.comp");
	
	addParamLine(ParamLine({ &alpha }));
	addParamLine(ParamLine({ &beta1 }));
	addParamLine(ParamLine({ &beta2 }));
	addParamLine(ParamLine({ &epsilon }));
}

void visual::AdamOptimizer::setParamNum(size_t paramNum)
{
	if (!optimizedData || optimizedData->getSize() != paramNum)
	{
		optimizedData = renderer::make_ssbo<float>(paramNum, GL_DYNAMIC_COPY);
		mVec = renderer::make_ssbo<float>(paramNum, GL_DYNAMIC_COPY);
		vVec = renderer::make_ssbo<float>(paramNum, GL_DYNAMIC_COPY);
	}
}

void visual::AdamOptimizer::reset()
{
	t = 0.0f;
	mVec->fillWithZeros();
	vVec->fillWithZeros();
}

void visual::AdamOptimizer::set(const renderer::ssbo_ptr<float> data)
{
	if (data->getSize() != optimizedData->getSize())
		throw std::runtime_error("AdamOptimizer::reset: initialData size does not match the current param num");
	optimizedData = data;
}

void visual::AdamOptimizer::optimize(const renderer::ssbo_ptr<float> g)
{
	if(g->getSize() != optimizedData->getSize())
		throw std::runtime_error("AdamOptimizer::optimize: gradient size does not match the optimizedData size");
	
	g->bindBuffer(19);

	t = t + 1.0f;
	(*adamOptimizerCompute)["alpha"] = alpha.value;
	(*adamOptimizerCompute)["beta1"] = beta1.value;
	(*adamOptimizerCompute)["beta2"] = beta2.value;
	(*adamOptimizerCompute)["epsilon"] = epsilon.value;
	(*adamOptimizerCompute)["t"] = t;
	optimizedData->bindBuffer(12);
	mVec->bindBuffer(13);
	vVec->bindBuffer(14);

	adamOptimizerCompute->dispatchCompute(optimizedData->getSize() / 64 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

renderer::ssbo_ptr<float> visual::AdamOptimizer::getOptimizedFloatData() const
{
	return optimizedData;
}
