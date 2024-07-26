#pragma once

#include "Renderer/Vulkan/Defines.h"

namespace vl::core::vulkan
{
	struct Frame
	{
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;

		VkSemaphore ImageAvailableSemaphore;
		VkSemaphore RenderFinishedSemaphore;
		VkFence Fence;
	};

	struct Swapchain
	{
		VkSwapchainKHR Handle = VK_NULL_HANDLE;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		VkExtent2D Extent = {};
		std::vector<VkImage> Images;
		std::vector<VkImageView> ImageViews;
	};

	struct Image
	{
		VkImage Handle;
		VkImageView View;
		VmaAllocation Allocation;
		VkExtent3D Extent;
		VkFormat Format;
	};

	struct DrawData
	{
		Image Image = {};
		VkExtent2D ImageExtent = {};
		vulkan::Image DepthImage = {};
	};

	struct ImmediateData
	{
		VkFence Fence;
		VkCommandBuffer CommandBuffer;
		VkCommandPool CommandPool;
	};

	struct AllocatedBuffer 
	{
		VkBuffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo Info;
	};

	struct BufferArray {

		AllocatedBuffer IndexBuffer;
		AllocatedBuffer VertexBuffer;
		VkDeviceAddress VertexBufferAddress;
	};
}