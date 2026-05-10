#include "fluidSurface.h"
#include "controlRegistry.h"
#include "engine/glUtils.hpp"


visual::FluidSurface::FluidSurface(glm::ivec2 resolution, std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights)
	: FluidRenderer(camera, lights)
{
	particleSpritesDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/visualization/3D/particle_sprites.vert", "shaders/visualization/3D/particle_sprites_depth.frag");
	gaussianBlurShaderX = std::make_shared<renderer::ShaderProgram>("shaders/utils/fullScreen.vert", "shaders/visualization/3D/gaussian_x.frag");
	gaussianBlurShaderY = std::make_shared<renderer::ShaderProgram>("shaders/utils/fullScreen.vert", "shaders/visualization/3D/gaussian_y.frag");
	shadedDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/utils/fullScreen.vert", "shaders/visualization/3D/shadedDepth.frag");
	bilateralFilterShaderX = std::make_shared<renderer::ShaderProgram>("shaders/utils/fullScreen.vert", "shaders/visualization/3D/bilateral_x.frag");
	bilateralFilterShaderY = std::make_shared<renderer::ShaderProgram>("shaders/utils/fullScreen.vert", "shaders/visualization/3D/bilateral_y.frag");
	fluidThicknessShader = std::make_shared<renderer::ShaderProgram>("shaders/visualization/3D/particle_sprites.vert", "shaders/visualization/3D/particle_sprites_thickness.frag");
	fluidThicknessBlurShader = std::make_shared<renderer::ShaderProgram>("shaders/utils/fullScreen.vert", "shaders/visualization/3D/gaussian_thickness.frag");
	normalAndDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/utils/fullScreen.vert", "shaders/visualization/3D/normal_depth.frag");

	initParamsProgram = std::make_shared<renderer::ComputeProgram>("shaders/visualization/3D/params/initParamBuffers.comp");
	xFillProgram = std::make_shared<renderer::ComputeProgram>("shaders/visualization/3D/params/x_fill.comp");
	sprayOverrideProgram = std::make_shared<renderer::ComputeProgram>("shaders/visualization/3D/params/sprayOverride.comp");

	prefixSum = std::make_unique<renderer::PrefixSum>();

	instancedParticles = std::make_unique<renderer::InstancedGeometry>
		(std::make_shared<renderer::FlipSquare>());

	fullScreenQuad = std::make_unique<renderer::Square>();

	{
		std::shared_ptr<renderer::RenderTargetTexture> depthTexture =
			std::make_shared<renderer::RenderTargetTexture>(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
		std::shared_ptr<renderer::RenderTargetTexture> depthParamTexture = std::make_shared<renderer::RenderTargetTexture>
			(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST, GL_R32I, GL_RED_INTEGER, GL_INT);
		depthFramebuffer = std::make_unique<renderer::Framebuffer>
			(renderer::Framebuffer::toArray({ depthParamTexture }), depthTexture, false);

		std::shared_ptr<renderer::RenderTargetTexture> depthBlurTmpTexture =
			std::make_shared<renderer::RenderTargetTexture>(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
		depthBlurTmpFramebuffer = std::make_unique<renderer::Framebuffer>
			(renderer::Framebuffer::toArray({}), depthBlurTmpTexture, false);

		finalDepthTexture =
			std::make_shared<renderer::RenderTargetTexture>(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
		finalDepthFramebuffer = std::make_unique<renderer::Framebuffer>
			(renderer::Framebuffer::toArray({}), finalDepthTexture, false);

		sprayParamTexture = std::make_shared<renderer::RenderTargetTexture>
			(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST, GL_R32I, GL_RED_INTEGER, GL_INT);
		sprayFramebuffer = std::make_unique<renderer::Framebuffer>
			(renderer::Framebuffer::toArray({ sprayParamTexture }), finalDepthTexture, false);

		std::shared_ptr<renderer::RenderTargetTexture> normalAndDepthTexture =
			std::make_shared<renderer::RenderTargetTexture>(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		normalAndDepthFramebuffer = std::make_unique<renderer::Framebuffer>
			(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>{ normalAndDepthTexture }, nullptr, false);
	}

	{
		std::shared_ptr<renderer::RenderTargetTexture> fluidThicknessTexture =
			std::make_shared<renderer::RenderTargetTexture>(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);
		fluidThicknessFramebuffer = std::make_unique<renderer::Framebuffer>
			(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>{ fluidThicknessTexture },
				nullptr, false);

		std::shared_ptr<renderer::RenderTargetTexture> fluidThicknessBlurTmpTexture =
			std::make_shared<renderer::RenderTargetTexture>(resolution.x, resolution.y, GL_NEAREST, GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);
		fluidThicknessBlurTmpFramebuffer = std::make_unique<renderer::Framebuffer>
			(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>{ fluidThicknessBlurTmpTexture }, nullptr, false);
	}

	(*shadedDepthShader)["normalAndDepthTexture"] = *normalAndDepthFramebuffer->getColorAttachments()[0];
	(*shadedDepthShader)["thicknessTexture"] = *fluidThicknessFramebuffer->getColorAttachments()[0];

	// Thickness and normalAndDepth read the FINAL Y-blurred depth.
	(*fluidThicknessShader)["depthTexture"] = *finalDepthTexture;
	(*normalAndDepthShader)["depthTexture"] = *finalDepthTexture;

	(*gaussianBlurShaderX)["paramTexture"] = *depthFramebuffer->getColorAttachments()[0];
	(*bilateralFilterShaderX)["paramTexture"] = *depthFramebuffer->getColorAttachments()[0];

	// X-fill only needs the surface paramTexture; it walks outward by index count and
	// derives the kernel reach from xCount, so no depth, camera or kernel-size uniforms
	// are required. sprayOverride flips slot 0 to the spray id afterwards.
	(*xFillProgram)["paramTexture"] = *depthFramebuffer->getColorAttachments()[0];
	(*sprayOverrideProgram)["sprayParamTexture"] = *sprayParamTexture;

	// X-blur reads sprite depth (depthFramebuffer.depth).
	(*gaussianBlurShaderX)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
	(*bilateralFilterShaderX)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
	// Y-blur reads X-blurred depth (depthBlurTmpFramebuffer.depth).
	(*gaussianBlurShaderY)["depthTexture"] = *depthBlurTmpFramebuffer->getDepthAttachment();
	(*bilateralFilterShaderY)["depthTexture"] = *depthBlurTmpFramebuffer->getDepthAttachment();

	(*particleSpritesDepthShader)["coloring.speedColorEnabled"] = false;
	(*fluidThicknessShader)["coloring.speedColorEnabled"] = false;


	camera->addProgram({ particleSpritesDepthShader, gaussianBlurShaderX, gaussianBlurShaderY,
		bilateralFilterShaderX, bilateralFilterShaderY, shadedDepthShader, fluidThicknessShader,
		fluidThicknessBlurShader, normalAndDepthShader });
	lights->addProgram({ particleSpritesDepthShader, shadedDepthShader, fluidThicknessShader });
	camera->setUniformsForAllPrograms();
	lights->setUniformsForAllPrograms();
}

void visual::FluidSurface::renderFluid(float dt, renderer::ssbo_ptr<genericfsim::manager::ParticleSSBOData> data,
	renderer::fb_ptr fb, pixel_params_ptr pixelParamBuffers)
{
	data->bindBuffer(1);

	const bool calculateParams = pixelParamBuffers != nullptr;
	const glm::ivec2 screenSize = depthFramebuffer->getSize();

	if (calculateParams)
	{
		if (pixelParamBuffers->getScreenSize() != screenSize)
			throw std::runtime_error("FluidSurface::renderFluid: pixelParamBuffers size mismatch");

		pixelParamBuffers->xCount->bindBuffer(20);
		pixelParamBuffers->xOffset->bindBuffer(21);
		pixelParamBuffers->xIndex->bindBuffer(22);
		pixelParamBuffers->yRadius->bindBuffer(23);

		const unsigned int pixelCount = pixelParamBuffers->getPixelCount();
		(*initParamsProgram)["pixelCount"] = pixelCount;
		initParamsProgram->dispatchCompute((pixelCount + 255) / 256, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	auto& registry = controls::ControlRegistry::getInstance();

	const bool transparencyEnabled = registry["render.fluid_transparency_en"];
	const float fluidTransparency = (float)registry["render.fluid_transparency"];
	const bool noiseEnabled = registry["render.fluid_surface_noise_en"];
	const float fluidSurfaceNoiseScale = (float)registry["render.fluid_surface_noise_scale"];
	const float fluidSurfaceNoiseStrength = (float)registry["render.fluid_surface_noise_strength"];
	const float fluidSurfaceNoiseSpeed = (float)registry["render.fluid_surface_noise_speed"];
	const float particleRadius = (float)registry["sim.particle_radius"];
	const bool sprayEnabled = registry["render.spray_en"];
	const float sprayDensityThreashold = (float)registry["render.spray_density_threshold"];
	const float smoothingSize = (float)registry["render.smoothing_size"];
	const bool bilateralFilterEnabled = registry["render.bilateral_filter_en"];
	const float blurScale = (float)registry["render.blur_scale"];
	const float blurDepthFalloff = (float)registry["render.blur_depth_falloff"];
	const glm::vec3 particleColor = (glm::vec3)registry["render.surface_color"];

	(*particleSpritesDepthShader)["particleRadius"] = particleRadius;
	(*particleSpritesDepthShader)["drawMode"] = sprayEnabled ? 1 : 0;
	(*particleSpritesDepthShader)["sprayThreashold"] = sprayDensityThreashold;

	// 1. Surface sprite depth pass.
	depthFramebuffer->bind();
	renderer::enableDepthTest(true);
	depthFramebuffer->clearDepthAttachment(1.0f);
	depthFramebuffer->clearColorAttachment(0, glm::ivec4(-1));

	instancedParticles->setInstanceNum(data->getSize());
	instancedParticles->draw();

	// 2. X-blur (writes X-blurred depth + xCount/xOffset).
	depthBlurTmpFramebuffer->bind();
	renderer::clearViewport(1.0f);

	const int xFilterRadiusCap = bilateralFilterEnabled ? 35 : 25;

	if (bilateralFilterEnabled)
	{
		(*bilateralFilterShaderX)["calculateParams"] = calculateParams;
		bilateralFilterShaderX->activate();
		(*bilateralFilterShaderX)["smoothingKernelSize"] = smoothingSize;
		(*bilateralFilterShaderX)["blurScale"] = blurScale;
		(*bilateralFilterShaderX)["blurDepthFalloff"] = blurDepthFalloff;
		fullScreenQuad->draw();
	}
	else
	{
		(*gaussianBlurShaderX)["calculateParams"] = calculateParams;
		gaussianBlurShaderX->activate();
		(*gaussianBlurShaderX)["smoothingKernelSize"] = smoothingSize;
		fullScreenQuad->draw();
	}
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// 3. Y-blur (writes Y-blurred depth to finalDepthFb, plus yRadius).
	finalDepthFramebuffer->bind();
	renderer::clearViewport(1.0f);
	if (bilateralFilterEnabled)
	{
		(*bilateralFilterShaderY)["calculateParams"] = calculateParams;
		bilateralFilterShaderY->activate();
		(*bilateralFilterShaderY)["smoothingKernelSize"] = smoothingSize;
		(*bilateralFilterShaderY)["blurScale"] = blurScale;
		(*bilateralFilterShaderY)["blurDepthFalloff"] = blurDepthFalloff;
		fullScreenQuad->draw();
	}
	else
	{
		(*gaussianBlurShaderY)["calculateParams"] = calculateParams;
		gaussianBlurShaderY->activate();
		(*gaussianBlurShaderY)["smoothingKernelSize"] = smoothingSize;
		fullScreenQuad->draw();
	}
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// 4. Prefix-scan xOffset (in-place inclusive) and X-fill BEFORE spray render.
	//    X-fill writes -1 to slot 0 (the spray sentinel) and surface IDs into slots
	//    1..count-1. sprayOverride later flips slot 0 to the spray id at spray pixels —
	//    crucially it leaves count/offset/surface IDs alone so they survive for
	//    neighbouring surface pixels' Y-walks.
	if (calculateParams)
	{
		// runInclusive binds to bindings 0 and 1, clobbering the particle SSBO at 1.
		prefixSum->runInclusive(pixelParamBuffers->xOffset);

		data->bindBuffer(1);
		pixelParamBuffers->xCount->bindBuffer(20);
		pixelParamBuffers->xOffset->bindBuffer(21);
		pixelParamBuffers->xIndex->bindBuffer(22);

		(*xFillProgram)["filterRadiusCap"] = xFilterRadiusCap;
		(*xFillProgram)["screenSize"] = screenSize;
		const glm::ivec3 xFillDispatch = glm::ivec3(
			(screenSize.x + 15) / 16,
			(screenSize.y + 15) / 16, 1);
		xFillProgram->dispatchCompute(xFillDispatch.x, xFillDispatch.y, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	// 5. Spray render: clear sprayParamTexture to -1; if sprayEnabled draw spray particles.
	//    The sprayFramebuffer shares its depth attachment with finalDepthFramebuffer, so
	//    spray sprites depth-test against the Y-blurred surface AND write their own depth
	//    into finalDepthTexture (matching the original visual semantics).
	sprayFramebuffer->bind();
	sprayFramebuffer->clearColorAttachment(0, glm::ivec4(-1));
	if (sprayEnabled)
	{
		(*particleSpritesDepthShader)["drawMode"] = 2;
		instancedParticles->draw();
	}

	// 6. Spray override: at spray pixels, overwrite slot 0 of the pixel's xIndex region
	//    (set to -1 by x_fill) with the spray particle id. Surface ids in slots 1..count-1
	//    are preserved.
	if (calculateParams)
	{
		(*sprayOverrideProgram)["screenSize"] = screenSize;
		const glm::ivec3 sprayDispatch = glm::ivec3(
			(screenSize.x + 31) / 32,
			(screenSize.y + 31) / 32, 1);
		sprayOverrideProgram->dispatchCompute(sprayDispatch.x, sprayDispatch.y, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	// 8. Thickness rendering (uses fb's depth attachment for transparency depth-test).
	if (transparencyEnabled)
	{
		fluidThicknessFramebuffer->setDepthAttachment(fb->getDepthAttachment());
		fluidThicknessFramebuffer->bind();
		fluidThicknessShader->activate();
		(*fluidThicknessShader)["particleRadius"] = particleRadius;
		renderer::clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_BLEND);
		instancedParticles->draw();
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);

		fluidThicknessBlurShader->activate();
		fluidThicknessBlurTmpFramebuffer->bind();
		renderer::enableDepthTest(false);
		renderer::clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		(*fluidThicknessBlurShader)["thicknessTexture"] = *fluidThicknessFramebuffer->getColorAttachments()[0];
		(*fluidThicknessBlurShader)["smoothingKernelSize"] = smoothingSize;
		(*fluidThicknessBlurShader)["axis"] = 0;
		(*fluidThicknessBlurShader)["depthTexture"] = *finalDepthTexture;
		fullScreenQuad->draw();

		fluidThicknessFramebuffer->bind();
		renderer::clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		(*fluidThicknessBlurShader)["thicknessTexture"] = *fluidThicknessBlurTmpFramebuffer->getColorAttachments()[0];
		(*fluidThicknessBlurShader)["axis"] = 1;
		fullScreenQuad->draw();

		renderer::enableDepthTest(true);
	}

	// 9. Normal + depth.
	normalAndDepthFramebuffer->bind();
	renderer::clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	normalAndDepthShader->activate();
	renderer::enableDepthTest(false);
	fullScreenQuad->draw();

	// 10. Final shaded composition.
	renderer::enableDepthTest(true);
	if (fb)
	{
		fb->bind();
		if (transparencyEnabled)
		{
			glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
			glEnable(GL_BLEND);
		}
		(*shadedDepthShader)["transparency"] = fluidTransparency;
		(*shadedDepthShader)["transparencyEnabled"] = transparencyEnabled;
		(*shadedDepthShader)["noiseEnabled"] = noiseEnabled;
		if (noiseEnabled)
		{
			(*shadedDepthShader)["noiseScale"] = fluidSurfaceNoiseScale;
			(*shadedDepthShader)["noiseStrength"] = fluidSurfaceNoiseStrength;
			(*shadedDepthShader)["noiseOffset"] = noiseOffset;
			noiseOffset += fluidSurfaceNoiseSpeed * 10.0f * dt;
		}
		(*shadedDepthShader)["object.diffuseColor"] = glm::vec4(particleColor, 1.0f);
		(*shadedDepthShader)["object.specularColor"] = glm::vec4(1.2, 1.2, 1.2, 1);
		(*shadedDepthShader)["object.shininess"] = 80.0f;
		fullScreenQuad->draw();
		glDisable(GL_BLEND);
	}

	renderer::bindDefaultFramebuffer();
}

void visual::FluidSurface::setScreenResolution(glm::ivec2 resolution)
{
	depthFramebuffer->setSize(resolution);
	depthBlurTmpFramebuffer->setSize(resolution);
	finalDepthFramebuffer->setSize(resolution);
	sprayFramebuffer->setSize(resolution);
	fluidThicknessFramebuffer->setSize(resolution);
	fluidThicknessBlurTmpFramebuffer->setSize(resolution);
	normalAndDepthFramebuffer->setSize(resolution);
}
