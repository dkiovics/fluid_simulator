#include "gradientSmoothing.h"

visual::GradientSmoothing::GradientSmoothing(float smoothingSphereR, glm::vec3 particleBox)
	: particleBox(particleBox), smoothingSphereR(smoothingSphereR), 
		particleCellCount(glm::ceil(particleBox / smoothingSphereR))
{ 
	smoothGradientCompute = renderer::make_compute("shaders/3D/gradientSmoothing/smoothGradient.comp");
	gradientCopyCompute = renderer::make_compute("shaders/3D/gradientSmoothing/gradientCopy.comp");

	(*smoothGradientCompute)["smoothingSphereR"] = smoothingSphereR;
	(*smoothGradientCompute)["particleCellCount"] = particleCellCount;
}

void visual::GradientSmoothing::smoothGradient
				(renderer::ssbo_ptr<GradientCalculatorInterface::ParticleGradientData> data)
{
	if (!particleIndexListSSBO)
	{
		size_t cellCount = particleCellCount.x * particleCellCount.y * particleCellCount.z;
		particleIndexListSSBO = renderer::make_ssbo<ParticleIndexList>(cellCount, GL_DYNAMIC_DRAW);
		particleGradientTmpSSBO = renderer::make_ssbo<glm::vec4>(data->getSize(), GL_DYNAMIC_DRAW);
	}
	particleGradientTmpSSBO->setSize(data->getSize());
	
	particleIndexListSSBO->fillWithZeros();

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
	data->mapBuffer(0, -1, GL_MAP_READ_BIT);
	particleIndexListSSBO->mapBuffer(0, -1, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
	for (int i = 0; i < data->getSize(); i++)
	{
		auto& particleData = (*data)[i];
		auto& indexList = (*particleIndexListSSBO)[getParticleIndex(particleData.position)];
		if (indexList.indexCount < indexList.indices.size())
		{
			indexList.indices[indexList.indexCount++] = i;
		}
	}
	data->unmapBuffer();
	particleIndexListSSBO->unmapBuffer();

	data->bindBuffer(0);
	particleIndexListSSBO->bindBuffer(1);
	particleGradientTmpSSBO->bindBuffer(2);
	smoothGradientCompute->dispatchCompute(data->getSize() / 256 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	gradientCopyCompute->dispatchCompute(data->getSize() / 256 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

size_t visual::GradientSmoothing::getParticleIndex(glm::vec3 particlePos) const
{
	glm::ivec3 index = particlePos / smoothingSphereR;
	index = glm::clamp(index, glm::ivec3(0), particleCellCount - 1);
	return index.x + index.y * particleCellCount.x + index.z * particleCellCount.x * particleCellCount.y;
}
