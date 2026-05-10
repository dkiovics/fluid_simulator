#include "fluidParticles.h"
#include "geometries/basicGeometries.h"
#include "controlRegistry.h"

using namespace visual;

visual::FluidParticles::FluidParticles(glm::ivec2 screenSize, std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights)
	: FluidRenderer(camera, lights)
{
	particleSpritesShader = std::make_shared<renderer::ShaderProgram>("shaders/visualization/3D/particle_sprites.vert", "shaders/visualization/3D/particle_sprites_shaded.frag");
	particleSprites = std::make_unique<renderer::Object3D<renderer::InstancedGeometry>>
		(std::make_shared<renderer::InstancedGeometry>(std::make_shared<renderer::FlipSquare>()), particleSpritesShader);
	particleSprites->shininess = 80.0f;
	particleSprites->specularColor = glm::vec4(1.2f, 1.2f, 1.2f, 1.0f);

	paramTmpTexture = std::make_shared<renderer::RenderTargetTexture>
		(screenSize.x, screenSize.y, GL_NEAREST, GL_NEAREST, GL_R32I, GL_RED_INTEGER, GL_INT);

	particleParamsProgram = std::make_unique<renderer::ComputeProgram>("shaders/visualization/3D/params/particles_count_fill.comp");
	(*particleParamsProgram)["paramTexture"] = *paramTmpTexture;
	(*particleParamsProgram)["screenSize"] = glm::ivec2(screenSize.x, screenSize.y);

	prefixSum = std::make_unique<renderer::PrefixSum>();

	camera->addProgram({ particleSpritesShader });
	lights->addProgram({ particleSpritesShader });
	camera->setUniformsForAllPrograms();
	lights->setUniformsForAllPrograms();
}

void visual::FluidParticles::renderFluid(float dt, renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
	renderer::fb_ptr fb, pixel_params_ptr pixelParamBuffers)
{
	data->bindBuffer(1);

	auto& registry = controls::ControlRegistry::getInstance();

	auto colorAttachment = fb->getColorAttachments()[0];
	fb->setColorAttachments(renderer::Framebuffer::toArray({ colorAttachment, paramTmpTexture }));
	fb->clearColorAttachment(1, glm::ivec4(-1));

	fb->bind();

	(*particleSpritesShader)["particleRadius"] = (float)registry["sim.particle_radius"];
	(*particleSpritesShader)["coloring.speedColorEnabled"] = (bool)registry["render.particle_speed_color_en"];
	(*particleSpritesShader)["coloring.maxColor"] = glm::vec4((glm::vec3)registry["render.particle_speed_color"], 1.0f);
	(*particleSpritesShader)["coloring.maxSpeed"] = (float)registry["render.max_particle_speed"];
	(*particleSpritesShader)["coloring.exponent"] = (float)registry["render.particle_speed_color_exp"];
	particleSprites->diffuseColor= glm::vec4((glm::vec3)registry["render.particle_color"], 1.0f);

	particleSprites->drawable->setInstanceNum(data->getSize());
	particleSprites->draw();

	fb->setColorAttachments(renderer::Framebuffer::toArray({ colorAttachment }));

	if (pixelParamBuffers)
	{
		const glm::ivec2 screenSize = paramTmpTexture->getSize();
		if (pixelParamBuffers->getScreenSize() != screenSize)
			throw std::runtime_error("FluidParticles::renderFluid: pixelParamBuffers size mismatch");

		pixelParamBuffers->xCount->bindBuffer(20);
		pixelParamBuffers->xOffset->bindBuffer(21);
		pixelParamBuffers->xIndex->bindBuffer(22);
		pixelParamBuffers->yRadius->bindBuffer(23);

		const glm::ivec3 dispatchSize = glm::ivec3(
			(screenSize.x + 15) / 16,
			(screenSize.y + 15) / 16, 1);

		// Phase 0: count + clear yRadius, write xCount and xOffset = (id >= 0 ? 1 : 0).
		(*particleParamsProgram)["phase"] = 0;
		particleParamsProgram->dispatchCompute(dispatchSize.x, dispatchSize.y, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Inclusive prefix scan on xOffset.
		// Note: prefix sum binds buffers at bindings 0/1, clobbering particle data.
		prefixSum->runInclusive(pixelParamBuffers->xOffset);

		// Re-bind for fill phase.
		pixelParamBuffers->xCount->bindBuffer(20);
		pixelParamBuffers->xOffset->bindBuffer(21);
		pixelParamBuffers->xIndex->bindBuffer(22);

		// Phase 1: fill xIndex.
		(*particleParamsProgram)["phase"] = 1;
		particleParamsProgram->dispatchCompute(dispatchSize.x, dispatchSize.y, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Re-bind particle data — prefix sum clobbered binding 1.
		data->bindBuffer(1);
	}
}

void visual::FluidParticles::setScreenResolution(glm::ivec2 resolution)
{
	paramTmpTexture->resizeTexture(resolution.x, resolution.y);
	(*particleParamsProgram)["screenSize"] = glm::ivec2(resolution.x, resolution.y);
}
