#pragma once
#include <vulkan/vulkan.h>
#include <span>
#include <vector>
#include "../Types.hpp"

namespace Vulkan::Common
{
    class DescriptorAllocator 
    {
    public:

        struct PoolSizeRatio {
            VkDescriptorType type;
            float ratio;
        };

        VkDescriptorPool Pool;

        inline void InitPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
        {
			std::vector<VkDescriptorPoolSize> poolSizes(poolRatios.size());
            for (size_t i = 0; i < poolRatios.size(); i++) 
            {
				poolSizes[i] =
				{
					.type = poolRatios[i].type,
					.descriptorCount = (uint32_t)(maxSets * poolRatios[i].ratio),
				};
            }

			VkDescriptorPoolCreateInfo info
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .flags = 0,
				.maxSets = maxSets,
				.poolSizeCount = (uint32_t)poolSizes.size(),
				.pPoolSizes = poolSizes.data(),
			};

			VK_CHECK(vkCreateDescriptorPool(device, &info, nullptr, &Pool));
        }

        inline void ClearDescriptors(VkDevice device)
        {
            VK_CHECK(vkResetDescriptorPool(device, Pool, 0));
        }

        inline void DestroyPool(VkDevice device)
        {
            vkDestroyDescriptorPool(device, Pool, nullptr);
        }

        inline VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout)
        {
			VkDescriptorSetAllocateInfo info
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = Pool,
				.descriptorSetCount = 1,
				.pSetLayouts = &layout,
			};

			VkDescriptorSet set;
			VK_CHECK(vkAllocateDescriptorSets(device, &info, &set));

			return set;
        }
    };
}