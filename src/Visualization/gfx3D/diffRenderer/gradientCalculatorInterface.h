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
		pertPlusFramebuffer = renderer::make_fb(
			renderer::Framebuffer::toArray({
				renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE)
				}),
			renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT),
			false
		);

		pertMinusFramebuffer = std::make_shared<renderer::Framebuffer>(
			renderer::Framebuffer::toArray({
				renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE)
				}),
			renderer::make_render_target(1000, 1000, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT),
			false
		);

		addParamLine({ &useDepthImage });
		addParamLine(ParamLine( { &depthErrorScale }, &useDepthImage ));

		adamGradientToParticleGradientProgram = renderer::make_compute("shaders/3D/diffRender/adamGradientToParticleGradient.comp");
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

	virtual void formatFloatParamsPreUpdate(renderer::ssbo_ptr<float> data) const { }

	virtual void formatFloatParamsPostUpdate(renderer::ssbo_ptr<float> data) const { }

	struct ParticleGradientData
	{
		glm::vec4 position;
		glm::vec4 gradient;
	};

	virtual void convertAdamGradientToParticleGradient(renderer::ssbo_ptr<float> adamGradient, 
		renderer::ssbo_ptr<ParticleGradientData> particleGradient) const 
	{
		adamGradient->bindBuffer(0);
		optimizedParamsSSBO->bindBuffer(1);
		particleGradient->bindBuffer(2);
		adamGradientToParticleGradientProgram->dispatchCompute(particleGradient->getSize() / 64 + 1, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	std::shared_ptr<ParamInterface> renderer3D;

protected:
	ParamBool useDepthImage = ParamBool("Use depth image", false);
	ParamFloat depthErrorScale = ParamFloat("Depth error scale", 1.0f, 0.0f, 20.0f);

	renderer::RenderEngine& renderEngine;
	std::shared_ptr<genericfsim::manager::SimulationManager> manager;

	renderer::fb_ptr pertPlusFramebuffer;
	renderer::fb_ptr pertMinusFramebuffer;

	renderer::ssbo_ptr<ParticleShaderData> paramNegativeOffsetSSBO;
	renderer::ssbo_ptr<ParticleShaderData> paramPositiveOffsetSSBO;
	renderer::ssbo_ptr<ParticleShaderData> optimizedParamsSSBO;
	renderer::ssbo_ptr<float> stochaisticGradientSSBO;

	renderer::compute_ptr adamGradientToParticleGradientProgram;
};

} // namespace visual
