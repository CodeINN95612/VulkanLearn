#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan::Init
{
	inline static VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = flags,
		};
	}

	inline static VkSemaphoreCreateInfo semaphoreCreateInfo()
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};
	}

	inline static VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = flags,
			.queueFamilyIndex = queueFamilyIndex,
		};
	}

	inline static VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool commandPool, uint32_t commandBufferCount, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPool,
			.level = level,
			.commandBufferCount = commandBufferCount,
		};
	}

	inline static VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer commandBuffer)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = commandBuffer,
			.deviceMask = 0,
		};
	}

	inline static VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkSemaphore semaphore, VkPipelineStageFlags2KHR stageMask)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = semaphore,
			.value = 1,
			.stageMask = stageMask,
			.deviceIndex = 0,
		};
	}

	inline static VkSubmitInfo2 submitInfo2(
		const VkCommandBufferSubmitInfo& commandBufferInfo, 
		const VkSemaphoreSubmitInfo& waitSemaphoreInfo, 
		const VkSemaphoreSubmitInfo& signalSemaphoreInfo)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &waitSemaphoreInfo,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &commandBufferInfo,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signalSemaphoreInfo,
		};
	}

	inline static VkPresentInfoKHR presentInfoKHR(VkSwapchainKHR* pSwapchain, uint32_t* pImageIndex, VkSemaphore* pSignalSemaphore)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = pSignalSemaphore,
			.swapchainCount = 1,
			.pSwapchains = pSwapchain,
			.pImageIndices = pImageIndex,
			.pResults = nullptr,
		};
	}

	inline static VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask)
	{
		return
		{
			.aspectMask = aspectMask,
			.baseMipLevel = 0,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.baseArrayLayer = 0,
			.layerCount = VK_REMAINING_ARRAY_LAYERS,
		};
	}

	inline static VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent = extent,
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = usageFlags,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
	}

	inline static VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,	
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.subresourceRange = Vulkan::Init::imageSubresourceRange(aspectFlags),
		};
	}
}
