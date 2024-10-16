#pragma once

#include <memory>
#include <engine/framebuffer.h>
#include <compute/computeProgram.h>
#include <compute/storageBuffer.h>
#include <param.hpp>
#include "gfx3D/renderer3DInterface.h"
#include "gfx3D/renderer/headers/paramInterface.h"

namespace visual
{

class GradientCalculatorInterface : public ParamLineCollection
{
public:
	GradientCalculatorInterface() : renderEngine(renderer::RenderEngine::getInstance()), renderer3D(nullptr)
	{
		addParamLine({ &useDepthImage });
		addParamLine(ParamLine( { &depthErrorScale }, &useDepthImage ));
		addParamLine({ &speedAbsPerturbation });
		addParamLine({ &posPerturbation });
	}

	/**
	 * \brief Update the optimized floats.
	 * \param data The data to update
	 * \param particleMovementAbs The absolute movement of the particles after the last update
	 */
	virtual void updateOptimizedFloats(renderer::ssbo_ptr<float> data, renderer::ssbo_ptr<float> particleMovementAbs) = 0;

	/**
	 * \brief Update the particle parameters that are being optimized.
	 * \param data The data to update
	 */
	virtual void updateParticleParams(renderer::ssbo_ptr<ParticleShaderData> data) = 0;

	/**
	* \brief Reset the gradient calculator.
	*/
	virtual void reset() = 0;

	/**
	* \brief Calculate the gradient of the current frame - must be called with after the parameter framebuffer is valid.
	* \param referenceFramebuffer The reference frame to compare with
	* \return The gradient that must be inputed to the optimizer
	*/
	virtual renderer::ssbo_ptr<float> calculateGradient(renderer::fb_ptr referenceFramebuffer) = 0;

	/**
	 * \brief Get the float parameters that are being optimized.
	 * \return The float parameters
	 */
	virtual renderer::ssbo_ptr<float> getFloatParams() = 0;

	/**
	 * \brief Get the optimized particle data based on the current optimized data.
	 * \return The particle data that is being optimized
	 */
	virtual renderer::ssbo_ptr<ParticleShaderData> getParticleData() = 0;

	/**
	 * \brief Returns the number of optimized parameters per particle.
	 * \return The number of optimized parameters per particle
	 */
	virtual size_t getOptimizedParamCountPerParticle() const = 0;

	std::shared_ptr<ParamInterface> renderer3D;

protected:
	ParamBool useDepthImage = ParamBool("Use depth image", false);
	ParamFloat depthErrorScale = ParamFloat("Depth error scale", 1.0f, 0.0f, 20.0f);
	ParamFloat speedAbsPerturbation = ParamFloat("Speed abs perturbation", 0.2f, 0.0f, 5.0f);
	ParamFloat posPerturbation = ParamFloat("Pos perturbation", 0.05f, 0.0f, 0.5f);

	renderer::RenderEngine& renderEngine;
};

} // namespace visual
