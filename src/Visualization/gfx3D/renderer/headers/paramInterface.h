#pragma once

#include <compute/storageBuffer.h>
#include <engineUtils/camera3D.hpp>
#include "gfx3D/renderer3DInterface.h"

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

	virtual std::shared_ptr<renderer::StorageBuffer<PixelParamData>> getParamBufferOut() const
	{
		return paramBufferOut;
	}

	virtual void invalidateParamBuffer()
	{
		paramBufferValid = false;
	}

	virtual std::shared_ptr<renderer::Camera3D> getCamera() const
	{
		return nullptr;
	}

protected:
	bool paramBufferValid = false;

	void setBufferSize(glm::ivec2 screenSize)
	{
		unsigned int size = screenSize.x * screenSize.y;
		if(paramBufferOut && paramBufferOut->getSize() == size)
			return;
		paramBufferOut = std::make_shared<renderer::StorageBuffer<PixelParamData>>
			(screenSize.x * screenSize.y, GL_DYNAMIC_COPY);
		paramBufferValid = false;
	}

	void deleteParamBuffer()
	{
		paramBufferValid = false;
		if(paramBufferOut)
			paramBufferOut.reset();
	}

private:
	std::shared_ptr<renderer::StorageBuffer<PixelParamData>> paramBufferOut;
};

} // namespace visual
