#pragma once

#include "Renderer/Vulkan/Defines.h"

namespace vl::core::vulkan
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
		const VkSemaphoreSubmitInfo* pWaitSemaphoreInfo,
		const VkSemaphoreSubmitInfo* pSignalSemaphoreInfo,
		uint32_t waitSemaphoreCount = 1,
		uint32_t signalSemaphoreCount = 1)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = pWaitSemaphoreInfo ? waitSemaphoreCount : 0,
			.pWaitSemaphoreInfos = pWaitSemaphoreInfo,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &commandBufferInfo,
			.signalSemaphoreInfoCount = pSignalSemaphoreInfo ? signalSemaphoreCount : 0,
			.pSignalSemaphoreInfos = pSignalSemaphoreInfo,
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
			.subresourceRange
			{
				.aspectMask = aspectFlags,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
		};
	}

	inline static VkRenderingAttachmentInfo colorAttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout)
	{
		VkRenderingAttachmentInfo colorAttachment
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = view,
			.imageLayout = layout,
			.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		};

		if (clear) {
			colorAttachment.clearValue = *clear;
		}

		return colorAttachment;
	}

	inline static VkRenderingAttachmentInfo depthAttachmentInfo(VkImageView view, VkImageLayout layout)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = view,
			.imageLayout = layout,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue
			{
				.depthStencil
				{
					.depth = 1.f,
				}
			}
		};
	}

	inline static VkRenderingInfo renderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.pNext = nullptr,
			.renderArea = VkRect2D{ VkOffset2D { 0, 0 }, renderExtent },
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = colorAttachment,
			.pDepthAttachment = depthAttachment,
			.pStencilAttachment = nullptr,
		};
	}

	inline static VkPipelineShaderStageCreateInfo  pipelineShaderStageCreateInfo(VkShaderModule shaderModule, VkShaderStageFlagBits stage)
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = stage,
			.module = shaderModule,
			.pName = "main",
		};
	}

	inline static VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo()
	{
		return
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
		};
	}
}