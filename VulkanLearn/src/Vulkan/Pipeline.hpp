#pragma once

#include <vulkan/vulkan.h>

#include "../Common/Utils.hpp"
#include "Types.hpp"

namespace Vulkan::Pipeline
{
	inline static VkShaderModule loadShaderModule(VkDevice device, const char* shaderPath)
	{
		std::vector<char> shaderCode = ::Common::Utils::readFile(shaderPath);

		VkShaderModuleCreateInfo createInfo
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = shaderCode.size(),
			.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data()),
		};

		VkShaderModule shaderModule;
		VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

		return shaderModule;
	}
}