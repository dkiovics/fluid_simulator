#include "densityControl.h"
#include <cstdlib>

visual::DensityControl::DensityControl()
{
	avgMovementCompute = renderer::make_compute("shaders/3D/densityControl/avgMovement.comp");
	initAvgMovementArray = renderer::make_compute("shaders/3D/densityControl/initAvgMovementArray.comp");
	updatePositionsCompute = renderer::make_compute("shaders/3D/densityControl/updatePositions.comp");

	addParamLine(ParamLine({ &sampleNum }));
	addParamLine(ParamLine({ &rollingAvgAlpha }));
	addParamLine(ParamLine({ &particlePercantageToMove }));
}

void visual::DensityControl::setParamNum(size_t paramNum)
{
	if (!avgMovement || avgMovement->getSize() != paramNum)
	{
		avgMovement = renderer::make_ssbo<AvgMovement>(paramNum, GL_DYNAMIC_COPY);
	}
	reset();
}

void visual::DensityControl::reset()
{
	sampleCount = 0;
	avgMovement->bindBuffer(0);
	initAvgMovementArray->dispatchCompute(avgMovement->getSize() / 64 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

bool visual::DensityControl::updateAvgMovement(renderer::ssbo_ptr<float> data)
{
	if (data->getSize() != avgMovement->getSize())
		throw std::runtime_error("DensityControl::updateAvgMovement: data size does not match the avgMovement size");

	avgMovement->bindBuffer(0);
	data->bindBuffer(1);
	(*avgMovementCompute)["rollingAvgAlpha"] = rollingAvgAlpha.value;
	avgMovementCompute->dispatchCompute(avgMovement->getSize() / 64 + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	sampleCount++;
	return sampleCount >= sampleNum.value;
}

renderer::ssbo_ptr<visual::DensityControl::AvgMovement> visual::DensityControl::getAvgMovementData()
{
	return avgMovement;
}

void visual::DensityControl::updatePositions(renderer::ssbo_ptr<visual::ParticleShaderData> data)
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	avgMovement->mapBuffer(0, -1, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
	std::sort(&(*avgMovement)[0], &(*avgMovement)[0] + avgMovement->getSize(),
				[](const AvgMovement& a, const AvgMovement& b) { return a.avgMovement < b.avgMovement; });
	avgMovement->unmapBuffer();

	avgMovement->bindBuffer(0);
	data->bindBuffer(1);
	(*updatePositionsCompute)["particleSpread"] = particleSpread.value;
	(*updatePositionsCompute)["seedX"] = float(std::rand()) / RAND_MAX;
	(*updatePositionsCompute)["seedY"] = float(std::rand()) / RAND_MAX;
	updatePositionsCompute->dispatchCompute(particlePercantageToMove.value / 100.0f * avgMovement->getSize(), 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	reset();
}
