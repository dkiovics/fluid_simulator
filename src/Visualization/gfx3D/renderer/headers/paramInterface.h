#pragma once

#include <compute/storageBuffer.h>
#include <engineUtils/camera3D.hpp>
#include <engineUtils/lights.hpp>
#include "gfx3D/renderer3DInterface.h"
#include <vector>

namespace visual
{

class ParamInterface : public Renderer3DInterface
{
public:
	struct PixelParamData
	{
		int paramNum;
		int paramIndexes[40];
	};

	virtual std::pair<std::shared_ptr<renderer::StorageBuffer<PixelParamData>>, bool> getParamBufferOut(int index) const
	{
		if (index >= paramBufferOut.size())
			return std::make_pair(nullptr, false);
		return paramBufferOut[index];
	}

	virtual void setParamBufferOutCnt(int cnt)
	{
		paramBufferOut.resize(cnt);
	}

	virtual void setActiveParamBuffer(int index)
	{
		activeParamBuffer = index;
	}

	virtual void invalidateParamBuffer()
	{
		for (auto& paramBuffer : paramBufferOut)
		{
			paramBuffer.second = false;
		}
	}

	virtual std::shared_ptr<renderer::Camera3D> getCamera() const
	{
		return nullptr;
	}

	virtual std::shared_ptr<renderer::Lights> getLights() const
	{
		return nullptr;
	}

	virtual void setParamBuffersRes(glm::ivec2 screenSize)
	{
		unsigned int size = screenSize.x * screenSize.y;
		for (auto& paramBuffer : paramBufferOut)
		{
			if (paramBuffer.first && paramBuffer.first->getSize() == size)
				continue;
			paramBuffer.first = std::make_shared<renderer::StorageBuffer<PixelParamData>>
				(screenSize.x * screenSize.y, GL_DYNAMIC_COPY);
			paramBuffer.second = false;
		}
	}

protected:
	int activeParamBuffer = 0;

	void deleteParamBuffer()
	{
		for (auto& paramBuffer : paramBufferOut)
		{
			paramBuffer.first = nullptr;
			paramBuffer.second = false;
		}
	}

	void setParamBufferValid()
	{
		if (activeParamBuffer >= paramBufferOut.size() || !paramBufferOut[activeParamBuffer].first)
			return;
		paramBufferOut[activeParamBuffer].second = true;
	}

	int getParamBufferCnt() const
	{
		return (int)paramBufferOut.size();
	}

private:
	std::vector<std::pair<std::shared_ptr<renderer::StorageBuffer<PixelParamData>>, bool>> paramBufferOut;
};

} // namespace visual
