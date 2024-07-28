#pragma once

namespace vl::core::vulkan
{
	inline static void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
	{
		VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
			? VK_IMAGE_ASPECT_DEPTH_BIT
			: VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageMemoryBarrier2 imageBarrier
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.pNext = nullptr,
			.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
			.oldLayout = currentLayout,
			.newLayout = newLayout,
			.image = image,
			.subresourceRange = imageSubresourceRange(aspectMask)
		};

		VkDependencyInfo depInfo
		{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &imageBarrier,
		};

		vkCmdPipelineBarrier2(cmd, &depInfo);
	}

	inline static void copyImage(VkCommandBuffer cmd, VkImage srcImage, VkImage dstImage, VkExtent2D srcSize, VkExtent2D dstSize)
	{
		VkImageBlit2 blitRegion
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
			.pNext = nullptr,
		};

		blitRegion.srcOffsets[1].x = srcSize.width;
		blitRegion.srcOffsets[1].y = srcSize.height;
		blitRegion.srcOffsets[1].z = 1;

		blitRegion.dstOffsets[1].x = dstSize.width;
		blitRegion.dstOffsets[1].y = dstSize.height;
		blitRegion.dstOffsets[1].z = 1;

		blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcSubresource.mipLevel = 0;

		blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.baseArrayLayer = 0;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstSubresource.mipLevel = 0;

		VkBlitImageInfo2 blitInfo
		{
			.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
			.pNext = nullptr,
			.srcImage = srcImage,
			.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.dstImage = dstImage,
			.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.regionCount = 1,
			.pRegions = &blitRegion,
			.filter = VK_FILTER_LINEAR,
		};

		vkCmdBlitImage2(cmd, &blitInfo);
	}

	inline static VkShaderModule createShaderModule(VkDevice device, const std::vector<uint32_t>& code, size_t codeSize)
	{
		VkShaderModuleCreateInfo createInfo
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = codeSize,
			.pCode = code.data(),
		};

		VkShaderModule shaderModule;
		VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

		return shaderModule;
	}
}