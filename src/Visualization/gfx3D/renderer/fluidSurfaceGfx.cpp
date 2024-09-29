#include "headers/fluidSurfaceGfx.h"

using namespace visual;

FluidSurfaceGfx::FluidSurfaceGfx(std::shared_ptr<renderer::RenderEngine> engine,
	std::shared_ptr<renderer::Camera3D> camera, std::shared_ptr<renderer::Lights> lights, unsigned int maxParticleNum)
	: engine(engine), camera(camera), lights(lights)
{
	particleSpritesDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/particle_sprites.vert", "shaders/particle_sprites_depth.frag");
	gaussianBlurShaderX = std::make_shared<renderer::ShaderProgram>("shaders/quad.vert", "shaders/gaussian_x.frag");
	gaussianBlurShaderY = std::make_shared<renderer::ShaderProgram>("shaders/quad.vert", "shaders/gaussian_y.frag");
	shadedDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vert", "shaders/shadedDepth.frag");
	bilateralFilterShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vert", "shaders/bilateral.frag");
	fluidThicknessShader = std::make_shared<renderer::ShaderProgram>("shaders/particle_sprites.vert", "shaders/particle_sprites_thickness.frag");
	fluidThicknessBlurShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vert", "shaders/gaussian_thickness.frag");
	normalAndDepthShader = std::make_shared<renderer::ShaderProgram>("shaders/quad.vert", "shaders/normal_depth.frag");

	surfaceSquareArrayObject = std::make_unique<renderer::Object3D<renderer::ParticleGeometryArray>>
		(std::make_shared<renderer::ParticleGeometryArray>(std::make_shared<renderer::FlipSquare>()), particleSpritesDepthShader);
	surfaceSquareArrayObject->drawable->setMaxInstanceNum(maxParticleNum);

	spraySquareArrayObject = std::make_unique<renderer::Object3D<renderer::ParticleGeometryArray>>
		(std::make_shared<renderer::ParticleGeometryArray>(std::make_shared<renderer::FlipSquare>()), particleSpritesDepthShader);
	spraySquareArrayObject->drawable->setMaxInstanceNum(maxParticleNum);

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

	camera->addProgram({ particleSpritesDepthShader, gaussianBlurShaderX, gaussianBlurShaderY,
		bilateralFilterShader, shadedDepthShader, fluidThicknessShader, fluidThicknessBlurShader, normalAndDepthShader });
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

void FluidSurfaceGfx::render(std::shared_ptr<renderer::Framebuffer> framebuffer, const Gfx3DRenderData& data)
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
	paramBufferOut->bindBuffer(30);

	fluidThicknessFramebuffer->setDepthAttachment(framebuffer->getDepthAttachment());

	updateParticleData(data);
	(*particleSpritesDepthShader)["particleRadius"] = config.particleRadius;

	depthFramebuffer->bind();
	engine->setViewport(0, 0, viewportSize.x, viewportSize.y);
	engine->enableDepthTest(true);
	depthFramebuffer->clearDepthAttachment(1.0f);
	depthFramebuffer->clearColorAttachment(0, glm::ivec4(-1));

	surfaceSquareArrayObject->draw();

	depthBlurTmpFramebuffer->bind();
	engine->clearViewport(1.0f);
	if (bilateralFilterEnabled.value)
	{
		bilateralFilterShader->activate();
		(*bilateralFilterShader)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
		(*bilateralFilterShader)["smoothingKernelSize"] = smoothingSize.value;
		(*bilateralFilterShader)["blurScale"] = blurScale.value;
		(*bilateralFilterShader)["blurDepthFalloff"] = blurDepthFalloff.value;
		(*bilateralFilterShader)["axis"] = 0;
		square->draw();

		depthFramebuffer->bind();
		engine->clearViewport(1.0f);
		(*bilateralFilterShader)["depthTexture"] = *depthBlurTmpFramebuffer->getDepthAttachment();
		(*bilateralFilterShader)["axis"] = 1;
		square->draw();
	}
	else
	{
		gaussianBlurShaderX->activate();
		(*gaussianBlurShaderX)["depthTexture"] = *depthFramebuffer->getDepthAttachment();
		(*gaussianBlurShaderX)["smoothingKernelSize"] = smoothingSize.value;
		square->draw();

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		depthFramebuffer->bind();
		engine->clearViewport(1.0f);
		gaussianBlurShaderY->activate();
		(*gaussianBlurShaderY)["depthTexture"] = *depthBlurTmpFramebuffer->getDepthAttachment();
		(*gaussianBlurShaderY)["smoothingKernelSize"] = smoothingSize.value;
		square->draw();
	}

	if (sprayEnabled.value)
	{
		depthFramebuffer->bind();
		spraySquareArrayObject->draw();
	}

	if (fluidTransparencyEnabled.value)
	{
		fluidThicknessFramebuffer->bind();
		fluidThicknessShader->activate();
		(*fluidThicknessShader)["particleRadius"] = config.particleRadius;
		engine->clearViewport(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_BLEND);
		surfaceSquareArrayObject->drawable->draw();
		if (sprayEnabled.value)
			spraySquareArrayObject->drawable->draw();
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

void FluidSurfaceGfx::updateParticleData(const Gfx3DRenderData& data)
{
	const int particleNum = data.particleData.size();
	auto surfaceSquareArray = surfaceSquareArrayObject->drawable;
	auto spraySquareArray = spraySquareArrayObject->drawable;

	if (sprayEnabled.value)
	{
		int surfaceParticleCount = 0;
		int sprayParticleCount = 0;
		for (int p = 0; p < particleNum; p++)
		{

			if (data.particleData[p].density < sprayDensityThreashold.value)
			{
				spraySquareArray->setOffset(sprayParticleCount, glm::vec4(data.particleData[p].pos, 1.0f));
				/*if (fluidSurfaceNoiseEnabled.value)
				{
					spraySquareArray->setSpeed(sprayParticleCount, particles[p].v);
					spraySquareArray->setId(sprayParticleCount, p);
				}*/
				sprayParticleCount++;
			}
			else
			{
				surfaceSquareArray->setOffset(surfaceParticleCount, glm::vec4(data.particleData[p].pos, 1.0f));
				/*if (fluidSurfaceNoiseEnabled.value)
				{
					surfaceSquareArray->setSpeed(surfaceParticleCount, particles[p].v);
					surfaceSquareArray->setId(surfaceParticleCount, p);
				}*/
				surfaceParticleCount++;
			}
		}
		spraySquareArray->setActiveInstanceNum(sprayParticleCount);
		surfaceSquareArray->setActiveInstanceNum(surfaceParticleCount);
		spraySquareArray->updateActiveInstanceParams();
		surfaceSquareArray->updateActiveInstanceParams();
	}
	else
	{
		for (int p = 0; p < particleNum; p++)
		{
			surfaceSquareArray->setOffset(p, glm::vec4(data.particleData[p].pos, 1.0f));
			/*if (fluidSurfaceNoiseEnabled.value)
			{
				surfaceSquareArray->setSpeed(p, particles[p].v);
				surfaceSquareArray->setId(p, p);
			}*/
		}
		surfaceSquareArray->setActiveInstanceNum(particleNum);
		surfaceSquareArray->updateActiveInstanceParams();
	}
}


