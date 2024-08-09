#pragma once

#include "Renderer/Vulkan/Types.h"

namespace vl::core::vulkan
{
	struct ChunkBufferData
	{
		AllocatedBuffer StorageBuffer;
		size_t DescriptorSetIndex;
	};
}
