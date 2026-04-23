#include "adam/adam.h"
#include "controlRegistry.h"

using namespace diffrender;


AdamOptimizer::AdamOptimizer()
{
	adamOptimizerCompute = renderer::make_compute("shaders/stateReconstruction/adam/adamOptimizer.comp");
}

void AdamOptimizer::setFluidBoxBounds(glm::vec3 lowerBound, glm::vec3 upperBound)
{
    (*adamOptimizerCompute)["lowerBound"] = lowerBound;
    (*adamOptimizerCompute)["upperBound"] = upperBound;
}

void AdamOptimizer::init(size_t particleNum)
{
    if (!mVec || mVec->getSize() != particleNum)
	{
        mVec = renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>((uint32_t) particleNum, GL_DYNAMIC_COPY);
        vVec = renderer::make_ssbo<genericfsim::manager::ParticleSSBOData>((uint32_t) particleNum, GL_DYNAMIC_COPY);
        particleMovementAbs = renderer::make_ssbo<float>((uint32_t) particleNum, GL_DYNAMIC_COPY);
	}
	t = 0.0f;
	mVec->fillWithZeros();
	vVec->fillWithZeros();
}

void AdamOptimizer::optimize(const renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
                             const renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> g)
{
	if(g->getSize() != data->getSize() || data->getSize() != mVec->getSize())
		throw std::runtime_error("AdamOptimizer::optimize: gradient size does not match the optimizedData size");

	auto& registry = controls::ControlRegistry::getInstance();
	const float alpha = registry["state.adam_alpha"];
	const float beta1 = registry["state.adam_beta1"];
	const float beta2 = registry["state.adam_beta2"];
	const float epsilon = registry["state.adam_epsilon"];
	
	g->bindBuffer(19);

	t = t + 1.0f;
	(*adamOptimizerCompute)["alpha"] = alpha;
	(*adamOptimizerCompute)["beta1"] = beta1;
	(*adamOptimizerCompute)["beta2"] = beta2;
	(*adamOptimizerCompute)["epsilon"] = epsilon;
	(*adamOptimizerCompute)["t"] = t;
	data->bindBuffer(12);
	mVec->bindBuffer(13);
	vVec->bindBuffer(14);
    particleMovementAbs->bindBuffer(2);
	
	adamOptimizerCompute->dispatchCompute(data->getSize() / 32 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

renderer::ssbo_ptr<float> AdamOptimizer::getLastParticleMovementAbs() const
{
    return particleMovementAbs;
}

