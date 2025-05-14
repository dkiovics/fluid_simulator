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

	paramCopyProgram = std::make_unique<renderer::ComputeProgram>("shaders/visualization/3D/params/copyParams_non_zero.comp");
	(*paramCopyProgram)["screenSize"] = resolution;

	depthParamBufferBlurX = std::make_unique<renderer::StorageBuffer<PixelParamDataX>>
		(resolution.x * resolution.y, GL_DYNAMIC_COPY);

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
	(*fluidThicknessShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
	(*normalAndDepthShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
	(*gaussianBlurShaderX)["paramTexture"] = *depthFramebuffer->getColorAttachments()[0];
	(*bilateralFilterShaderX)["paramTexture"] = *depthFramebuffer->getColorAttachments()[0];
	(*paramCopyProgram)["paramTexture"] = *depthFramebuffer->getColorAttachments()[0];
	(*gaussianBlurShaderX)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
	(*gaussianBlurShaderY)["depthTexture"] = *depthBlurTmpFramebuffer->getDepthAttachment();
	(*bilateralFilterShaderX)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
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
	renderer::fb_ptr fb, renderer::ssbo_ptr<PixelParamData> pixelParamData)
{
	data->bindBuffer(1);
	depthParamBufferBlurX->bindBuffer(20);
	if (pixelParamData)
		pixelParamData->bindBuffer(30);

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
	
	depthFramebuffer->bind();
	renderer::enableDepthTest(true);
	depthFramebuffer->clearDepthAttachment(1.0f);
	depthFramebuffer->clearColorAttachment(0, glm::ivec4(-1));

	instancedParticles->setInstanceNum(data->getSize());
	instancedParticles->draw();

	depthBlurTmpFramebuffer->bind();
	renderer::clearViewport(1.0f);
	if (bilateralFilterEnabled)
	{
		if (pixelParamData)
		{
			(*bilateralFilterShaderX)["calculateParams"] = true;
			(*bilateralFilterShaderY)["calculateParams"] = true;
		}
		else
		{
			(*bilateralFilterShaderX)["calculateParams"] = false;
			(*bilateralFilterShaderY)["calculateParams"] = false;
		}

		bilateralFilterShaderX->activate();
		(*bilateralFilterShaderX)["smoothingKernelSize"] = smoothingSize;
		(*bilateralFilterShaderX)["blurScale"] = blurScale;
		(*bilateralFilterShaderX)["blurDepthFalloff"] = blurDepthFalloff;
		fullScreenQuad->draw();

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		depthFramebuffer->bind();
		renderer::clearViewport(1.0f);
		bilateralFilterShaderY->activate();
		(*bilateralFilterShaderY)["smoothingKernelSize"] = smoothingSize;
		(*bilateralFilterShaderY)["blurScale"] = blurScale;
		(*bilateralFilterShaderY)["blurDepthFalloff"] = blurDepthFalloff;
		fullScreenQuad->draw();

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
	else
	{
		if (pixelParamData)
		{
			(*gaussianBlurShaderX)["calculateParams"] = true;
			(*gaussianBlurShaderY)["calculateParams"] = true;
		}
		else
		{
			(*gaussianBlurShaderX)["calculateParams"] = false;
			(*gaussianBlurShaderY)["calculateParams"] = false;
		}

		gaussianBlurShaderX->activate();
		(*gaussianBlurShaderX)["smoothingKernelSize"] = smoothingSize;
		fullScreenQuad->draw();

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		depthFramebuffer->bind();
		renderer::clearViewport(1.0f);
		gaussianBlurShaderY->activate();
		(*gaussianBlurShaderY)["smoothingKernelSize"] = smoothingSize;
		fullScreenQuad->draw();

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	if (sprayEnabled)
	{
		depthFramebuffer->bind();
		depthFramebuffer->clearColorAttachment(0, glm::ivec4(-1));
		(*particleSpritesDepthShader)["drawMode"] = 2;
		instancedParticles->draw();
		if (pixelParamData)
		{
			glm::ivec3 dispatchSize = glm::ivec3(
				ceil((float)depthParamBufferBlurX->getSize() / 32.0f),
				ceil((float)depthParamBufferBlurX->getSize() / 32.0f), 1);
			paramCopyProgram->dispatchCompute(dispatchSize.x, dispatchSize.y, 1);
		}
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	if (registry["render.fluid_transparency_en"])
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
		(*fluidThicknessBlurShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
		fullScreenQuad->draw();

		fluidThicknessFramebuffer->bind();
		renderer::clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		(*fluidThicknessBlurShader)["thicknessTexture"] = *fluidThicknessBlurTmpFramebuffer->getColorAttachments()[0];
		(*fluidThicknessBlurShader)["axis"] = 1;
		fullScreenQuad->draw();

		renderer::enableDepthTest(true);
	}

	normalAndDepthFramebuffer->bind();
	renderer::clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	normalAndDepthShader->activate();
	renderer::enableDepthTest(false);
	fullScreenQuad->draw();

	renderer::enableDepthTest(true);
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

	renderer::bindDefaultFramebuffer();
}

void visual::FluidSurface::setScreenResolution(glm::ivec2 resolution)
{
	depthFramebuffer->setSize(resolution);
	depthBlurTmpFramebuffer->setSize(resolution);
	fluidThicknessFramebuffer->setSize(resolution);
	fluidThicknessBlurTmpFramebuffer->setSize(resolution);
	normalAndDepthFramebuffer->setSize(resolution);
	(*paramCopyProgram)["screenSize"] = resolution;
	depthParamBufferBlurX->setSize(resolution.x * resolution.y);
}
