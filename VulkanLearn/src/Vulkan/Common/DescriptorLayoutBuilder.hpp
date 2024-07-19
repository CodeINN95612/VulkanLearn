#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "../Types.hpp"

namespace Vulkan::Common
{
    class DescriptorLayoutBuilder 
    {
    public:

        std::vector<VkDescriptorSetLayoutBinding> Bindings;

        inline void AddBinding(uint32_t binding, VkDescriptorType type)
        {
            VkDescriptorSetLayoutBinding newbind
            {
                .binding = binding,
                .descriptorType = type,
                .descriptorCount = 1,
            };

            Bindings.push_back(newbind);
        }

        inline void Clear()
        {
            Bindings.clear();
        }

        inline VkDescriptorSetLayout Build(
            VkDevice device,
            VkShaderStageFlags shaderStages,
            void* pNext = nullptr,
            VkDescriptorSetLayoutCreateFlags flags = 0)
        {
            for (auto& b : Bindings) {
                b.stageFlags |= shaderStages;
            }

            VkDescriptorSetLayoutCreateInfo info 
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = pNext,
                .flags = flags,
                .bindingCount = (uint32_t)Bindings.size(),
                .pBindings = Bindings.data(),
            };

            VkDescriptorSetLayout set;
            VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

            return set;
        }
    };
}