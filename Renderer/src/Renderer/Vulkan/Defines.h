#pragma once

#define NOMINMAX
#define GLM_ENABLE_EXPERIMENTAL
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR

#include <exception>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <vma/vk_mem_alloc.h>

#if DEBUG
#define VALIDATION_LAYERS_ENABLED true
#else
#define VALIDATION_LAYERS_ENABLED false
#endif

#define MAX_FRAMES_IN_FLIGHT 2

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