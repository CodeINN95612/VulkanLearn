#pragma once

#include <string>
#include <exception>
#include <vulkan/vk_enum_string_helper.h>

#include <spdlog/spdlog.h>
#include <vma/vk_mem_alloc.h>

#define VK_CHECK(f) \
{ \
	VkResult res = (f); \
	if (res != VK_SUCCESS) \
	{ \
		std::string error = "VkResult is " +  std::string(string_VkResult(res)) + " in " + std::string(__FILE__) + " at " + std::to_string(__LINE__); \
		spdlog::error("VkResult is {0} in {1} at line {2}", string_VkResult(res), __FILE__, __LINE__); \
		throw std::exception(error.c_str()); \
	} \
}

namespace Vulkan
{
	struct AllocatedBuffer {
		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo info;
	};

	struct Vertex {
		glm::vec3 position;
		float uv_x;
		glm::vec3 normal;
		float uv_y;
		glm::vec4 color;
	};

	struct GPUMeshBuffers {

		AllocatedBuffer indexBuffer;
		AllocatedBuffer vertexBuffer;
		VkDeviceAddress vertexBufferAddress;
	};

	struct GPUDrawPushConstants {
		glm::mat4 modelMatrix;
		VkDeviceAddress vertexBuffer;
	};
}