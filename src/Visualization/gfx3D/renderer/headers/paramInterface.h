#pragma once

#include <compute/storageBuffer.h>
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

protected:
	void setBufferSize(glm::ivec2 screenSize)
	{
		unsigned int size = screenSize.x * screenSize.y;
		if(paramBufferOut && paramBufferOut->getSize() == size)
			return;
		paramBufferOut = std::make_shared<renderer::StorageBuffer<PixelParamData>>
			(screenSize.x * screenSize.y, GL_DYNAMIC_COPY);
	}
	std::shared_ptr<renderer::StorageBuffer<PixelParamData>> paramBufferOut;
};

} // namespace visual
