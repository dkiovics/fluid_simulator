#include "headers/fluidSurfaceGfx.h"

using namespace visual;

FluidSurfaceGfx::FluidSurfaceGfx(std::shared_ptr<renderer::RenderEngine> engine,
	std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights, unsigned int maxParticleNum)
	: engine(engine), camera(camera), lights(lights)
{
	particleSpritesDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/3D/surface/particle_sprites.vert", "shaders/3D/surface/particle_sprites_depth.frag");
	gaussianBlurShaderX = std::make_shared<renderer::ShaderProgram>("shaders/3D/util/quad.vert", "shaders/3D/surface/gaussian_x.frag");
	gaussianBlurShaderY = std::make_shared<renderer::ShaderProgram>("shaders/3D/util/quad.vert", "shaders/3D/surface/gaussian_y.frag");
	shadedDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/3D/util/quad.vert", "shaders/3D/surface/shadedDepth.frag");
	bilateralFilterShaderX = std::make_shared<renderer::ShaderProgram>("shaders/3D/util/quad.vert", "shaders/3D/surface/bilateral_x.frag");
	bilateralFilterShaderY = std::make_shared<renderer::ShaderProgram>("shaders/3D/util/quad.vert", "shaders/3D/surface/bilateral_y.frag");
	fluidThicknessShader = std::make_shared<renderer::ShaderProgram>("shaders/3D/surface/particle_sprites.vert", "shaders/3D/surface/particle_sprites_thickness.frag");
	fluidThicknessBlurShader = std::make_shared<renderer::ShaderProgram>("shaders/3D/util/quad.vert", "shaders/3D/surface/gaussian_thickness.frag");
	normalAndDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/3D/util/quad.vert", "shaders/3D/surface/normal_depth.frag");
	paramCopyCompute = renderer::make_compute("shaders/3D/util/copyParams_non_zero.comp");

	instancedParticles = std::make_unique<renderer::Object3D<renderer::InstancedGeometry>>
		(std::make_shared<renderer::InstancedGeometry>(std::make_shared<renderer::FlipSquare>()), particleSpritesDepthShader);

	shadedSquareObject = std::make_unique<renderer::Object3D<renderer::Square>>(std::make_shared<renderer::Square>(), shadedDepthShader);
	shadedSquareObject->shininess = 80;
	shadedSquareObject->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);

	square = std::make_unique<renderer::Square>();

	{
		std::shared_ptr<renderer::RenderTargetTexture> depthTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
		std::shared_ptr<renderer::RenderTargetTexture> depthParamTexture = std::make_shared<renderer::RenderTargetTexture>
			(1000, 1000, GL_NEAREST, GL_NEAREST, GL_R32I, GL_RED_INTEGER, GL_INT);
		depthFramebuffer = std::make_unique<renderer::Framebuffer>
			(renderer::Framebuffer::toArray( {depthParamTexture} ), depthTexture, false);

		std::shared_ptr<renderer::RenderTargetTexture> depthBlurTmpTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
		depthBlurTmpFramebuffer = std::make_unique<renderer::Framebuffer>
			(renderer::Framebuffer::toArray({}), depthBlurTmpTexture, false);

		std::shared_ptr<renderer::RenderTargetTexture> normalAndDepthTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_RGBA32F, GL_RGBA, GL_FLOAT);
		normalAndDepthFramebuffer = std::make_unique<renderer::Framebuffer>
			(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>{ normalAndDepthTexture }, nullptr, false);
	}

	{
		std::shared_ptr<renderer::RenderTargetTexture> fluidThicknessTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);
		fluidThicknessFramebuffer = std::make_unique<renderer::Framebuffer>
			(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>{ fluidThicknessTexture },
				nullptr, false);

		std::shared_ptr<renderer::RenderTargetTexture> fluidThicknessBlurTmpTexture =
			std::make_shared<renderer::RenderTargetTexture>(1000, 1000, GL_NEAREST, GL_NEAREST, GL_R32F, GL_RED, GL_FLOAT);
		fluidThicknessBlurTmpFramebuffer = std::make_unique<renderer::Framebuffer>
			(std::vector<std::shared_ptr<renderer::RenderTargetTexture>>{ fluidThicknessBlurTmpTexture }, nullptr, false);
	}

	(*shadedDepthShader)["normalAndDepthTexture"] = *normalAndDepthFramebuffer->getColorAttachments()[0];
	(*shadedDepthShader)["thicknessTexture"] = *fluidThicknessFramebuffer->getColorAttachments()[0];
	(*fluidThicknessShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
	(*normalAndDepthShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
	(*gaussianBlurShaderX)["paramTexture"] = *depthFramebuffer->getColorAttachments()[0];
	(*bilateralFilterShaderX)["paramTexture"] = *depthFramebuffer->getColorAttachments()[0];
	(*paramCopyCompute)["paramTexture"] = *depthFramebuffer->getColorAttachments()[0];
	(*gaussianBlurShaderX)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
	(*gaussianBlurShaderY)["depthTexture"] = *depthBlurTmpFramebuffer->getDepthAttachment();
	(*bilateralFilterShaderX)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
	(*bilateralFilterShaderY)["depthTexture"] = *depthBlurTmpFramebuffer->getDepthAttachment();


	camera->addProgram({ particleSpritesDepthShader, gaussianBlurShaderX, gaussianBlurShaderY,
		bilateralFilterShaderX, bilateralFilterShaderY, shadedDepthShader, fluidThicknessShader, 
		fluidThicknessBlurShader, normalAndDepthShader });
	lights->addProgram({ particleSpritesDepthShader, shadedDepthShader, fluidThicknessShader });
	camera->setUniformsForAllPrograms();
	lights->setUniformsForAllPrograms();

	addParamLine(ParamLine({ &particleColor, &smoothingSize }));
	addParamLine(ParamLine({ &bilateralFilterEnabled }));
	addParamLine(ParamLine({ &blurScale, &blurDepthFalloff }, &bilateralFilterEnabled));
	addParamLine(ParamLine({ &sprayEnabled }));
	addParamLine(ParamLine({ &sprayDensityThreashold }, &sprayEnabled));
	addParamLine(ParamLine({ &fluidTransparencyEnabled }));
	addParamLine(ParamLine({ &fluidTransparencyBlurSize, &fluidTransparency }, &fluidTransparencyEnabled));
	addParamLine(ParamLine({ &fluidSurfaceNoiseEnabled }));
	addParamLine(ParamLine({ &fluidSurfaceNoiseScale, &fluidSurfaceNoiseStrength }, &fluidSurfaceNoiseEnabled));
	addParamLine(ParamLine({ &fluidSurfaceNoiseSpeed }, &fluidSurfaceNoiseEnabled));
}

void FluidSurfaceGfx::setConfigData(const ConfigData3D& config)
{
	this->config = config;
}

void FluidSurfaceGfx::render(std::shared_ptr<renderer::Framebuffer> framebuffer, renderer::ssbo_ptr<ParticleShaderData> data)
{
	glm::ivec2 viewportSize = framebuffer->getSize();
	if (viewportSize != depthFramebuffer->getSize())
	{
		depthFramebuffer->setSize(viewportSize);
		depthBlurTmpFramebuffer->setSize(viewportSize);
		fluidThicknessBlurTmpFramebuffer->setSize(viewportSize);
		fluidThicknessFramebuffer->setSize(viewportSize);
		normalAndDepthFramebuffer->setSize(viewportSize);
		depthParamBufferBlurX = std::make_unique<renderer::StorageBuffer<PixelParamDataX>>
			(viewportSize.x * viewportSize.y, GL_DYNAMIC_COPY);
		setBufferSize(viewportSize);
		(*particleSpritesDepthShader)["resolution"] = viewportSize;
	}

	depthParamBufferBlurX->bindBuffer(20);
	getParamBufferOut()->bindBuffer(30);
	data->bindBuffer(80);

	fluidThicknessFramebuffer->setDepthAttachment(framebuffer->getDepthAttachment());

	(*particleSpritesDepthShader)["particleRadius"] = config.particleRadius;
	(*particleSpritesDepthShader)["drawMode"] = sprayEnabled.value ? 1 : 0;
	(*particleSpritesDepthShader)["sprayThreashold"] = sprayDensityThreashold.value;
	instancedParticles->drawable->setInstanceNum(data->getSize());

	depthFramebuffer->bind();
	engine->setViewport(0, 0, viewportSize.x, viewportSize.y);
	engine->enableDepthTest(true);
	depthFramebuffer->clearDepthAttachment(1.0f);
	depthFramebuffer->clearColorAttachment(0, glm::ivec4(-1));

	instancedParticles->draw();

	depthBlurTmpFramebuffer->bind();
	engine->clearViewport(1.0f);
	if (bilateralFilterEnabled.value)
	{
		if (paramBufferValid)
		{
			(*bilateralFilterShaderX)["calculateParams"] = false;
			(*bilateralFilterShaderY)["calculateParams"] = false;
		}
		else
		{
			(*bilateralFilterShaderX)["calculateParams"] = true;
			(*bilateralFilterShaderY)["calculateParams"] = true;
		}

		bilateralFilterShaderX->activate();
		(*bilateralFilterShaderX)["smoothingKernelSize"] = smoothingSize.value;
		(*bilateralFilterShaderX)["blurScale"] = blurScale.value;
		(*bilateralFilterShaderX)["blurDepthFalloff"] = blurDepthFalloff.value;
		square->draw();

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		depthFramebuffer->bind();
		engine->clearViewport(1.0f);
		bilateralFilterShaderX->activate();
		(*bilateralFilterShaderY)["smoothingKernelSize"] = smoothingSize.value;
		(*bilateralFilterShaderY)["blurScale"] = blurScale.value;
		(*bilateralFilterShaderY)["blurDepthFalloff"] = blurDepthFalloff.value;
		square->draw();
	}
	else
	{
		if (paramBufferValid)
		{
			(*gaussianBlurShaderX)["calculateParams"] = false;
			(*gaussianBlurShaderY)["calculateParams"] = false;
		}
		else
		{
			(*gaussianBlurShaderX)["calculateParams"] = true;
			(*gaussianBlurShaderY)["calculateParams"] = true;
		}
		gaussianBlurShaderX->activate();
		(*gaussianBlurShaderX)["smoothingKernelSize"] = smoothingSize.value;
		square->draw();

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		depthFramebuffer->bind();
		engine->clearViewport(1.0f);
		gaussianBlurShaderY->activate();
		(*gaussianBlurShaderY)["smoothingKernelSize"] = smoothingSize.value;
		square->draw();
	}

	if (sprayEnabled.value)
	{
		depthFramebuffer->bind();
		depthFramebuffer->clearColorAttachment(0, glm::ivec4(-1));
		(*particleSpritesDepthShader)["drawMode"] = 2;
		instancedParticles->draw();
		if (!paramBufferValid)
		{
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			(*paramCopyCompute)["screenSize"] = viewportSize;
			paramCopyCompute->dispatchCompute(viewportSize.x, viewportSize.y, 1);
		}
	}

	paramBufferValid = true;

	if (fluidTransparencyEnabled.value)
	{
		fluidThicknessFramebuffer->bind();
		fluidThicknessShader->activate();
		(*fluidThicknessShader)["particleRadius"] = config.particleRadius;
		engine->clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_BLEND);
		instancedParticles->drawable->draw();
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);

		fluidThicknessBlurShader->activate();
		fluidThicknessBlurTmpFramebuffer->bind();
		engine->enableDepthTest(false);
		engine->clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		(*fluidThicknessBlurShader)["thicknessTexture"] = *fluidThicknessFramebuffer->getColorAttachments()[0];
		(*fluidThicknessBlurShader)["smoothingKernelSize"] = fluidTransparencyBlurSize.value;
		(*fluidThicknessBlurShader)["axis"] = 0;
		(*fluidThicknessBlurShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
		square->draw();

		fluidThicknessFramebuffer->bind();
		engine->clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		(*fluidThicknessBlurShader)["thicknessTexture"] = *fluidThicknessBlurTmpFramebuffer->getColorAttachments()[0];
		(*fluidThicknessBlurShader)["axis"] = 1;
		square->draw();

		engine->enableDepthTest(true);
	}

	normalAndDepthFramebuffer->bind();
	engine->clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	normalAndDepthShader->activate();
	engine->enableDepthTest(false);
	square->draw();

	engine->enableDepthTest(true);
	framebuffer->bind();
	if (fluidTransparencyEnabled.value)
	{
		glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
		glEnable(GL_BLEND);
	}
	(*shadedDepthShader)["transparency"] = fluidTransparency.value;
	(*shadedDepthShader)["transparencyEnabled"] = fluidTransparencyEnabled.value;
	(*shadedDepthShader)["noiseEnabled"] = fluidSurfaceNoiseEnabled.value;
	if (fluidSurfaceNoiseEnabled.value)
	{
		(*shadedDepthShader)["noiseScale"] = fluidSurfaceNoiseScale.value;
		(*shadedDepthShader)["noiseStrength"] = fluidSurfaceNoiseStrength.value;
		(*shadedDepthShader)["noiseOffset"] = noiseOffset;
		noiseOffset += fluidSurfaceNoiseSpeed.value * 10.0 * engine->getLastFrameTime();
	}
	shadedSquareObject->diffuseColor = glm::vec4(particleColor.value, 1.0f);
	shadedSquareObject->draw();
	glDisable(GL_BLEND);

	engine->bindDefaultFramebuffer();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}


