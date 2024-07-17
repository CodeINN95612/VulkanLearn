#pragma once

#include <vk_boostrap/VkBootstrap.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include "Types.hpp"

namespace Vulkan
{
	struct BoostrapData
	{
		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
		VkQueue graphicsQueue;
		uint32_t graphicsQueueFamilyIndex;
		VkQueue presentQueue;
		uint32_t presentQueueFamilyIndex;
	};

	inline static BoostrapData boostrapVulkan(GLFWwindow* pWindow, 
		PFN_vkDebugUtilsMessengerCallbackEXT debugMessengerCallback, 
		bool useValidationLayers)
	{
		const char* appName = glfwGetWindowTitle(pWindow);

		vkb::InstanceBuilder builder;

		//make the vulkan instance, with basic debug features
		auto inst_ret = builder.set_app_name(appName)
			.request_validation_layers(useValidationLayers)
			.set_debug_callback(debugMessengerCallback)
			.require_api_version(1, 3, 0)
			.build();


		vkb::Instance vkb_inst = inst_ret.value();
		VkInstance instance = vkb_inst.instance;
		VkDebugUtilsMessengerEXT debugMessenger = vkb_inst.debug_messenger;

		VkSurfaceKHR surface;
		VK_CHECK(glfwCreateWindowSurface(instance, pWindow, nullptr, &surface));

		VkPhysicalDeviceFeatures features{};
		features.samplerAnisotropy = VK_TRUE;

		VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		features13.dynamicRendering = true;
		features13.synchronization2 = true;

		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;

		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		auto physicalDevice_ret = selector
			.allow_any_gpu_device_type(false)
			.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
			.set_minimum_version(1, 3)
			.set_required_features(features)
			.set_required_features_13(features13)
			.set_required_features_12(features12)
			.set_surface(surface)
			.select();

		vkb::PhysicalDevice vkb_physicalDevice = physicalDevice_ret.value();
		VkPhysicalDevice physicalDevice = vkb_physicalDevice.physical_device;

		vkb::DeviceBuilder deviceBuilder{ vkb_physicalDevice };
		auto vkbDevice_ret = deviceBuilder.build();

		vkb::Device vkb_device = vkbDevice_ret.value();
		VkDevice logicalDevice = vkb_device.device;

		auto graphicsQueue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
		VkQueue graphicsQueue = graphicsQueue_ret.value();

		auto graphicsQueueFamily_ret = vkb_device.get_queue_index(vkb::QueueType::graphics);
		uint32_t graphicsQueueFamilyIndex = graphicsQueueFamily_ret.value();

		auto presentQueue_ret = vkb_device.get_queue(vkb::QueueType::present);
		VkQueue presentQueue = presentQueue_ret.value();

		auto presentQueueFamily_ret = vkb_device.get_queue_index(vkb::QueueType::present);
		uint32_t presentQueueFamilyIndex = presentQueueFamily_ret.value();

		return
		{
			.instance = instance,
			.debugMessenger = debugMessenger,
			.surface = surface,
			.physicalDevice = physicalDevice,
			.logicalDevice = logicalDevice,
			.graphicsQueue = graphicsQueue,
			.graphicsQueueFamilyIndex = graphicsQueueFamilyIndex,
			.presentQueue = presentQueue,
			.presentQueueFamilyIndex = presentQueueFamilyIndex
		};
	}

	inline static void cleanBoostrapedData(const BoostrapData& data)
	{
		vkDestroyDevice(data.logicalDevice, nullptr);
		vkDestroySurfaceKHR(data.instance, data.surface, nullptr);
		vkb::destroy_debug_utils_messenger(data.instance, data.debugMessenger, nullptr);
		vkDestroyInstance(data.instance, nullptr);
	}

	
	struct BoostrapSwapchainData
	{
		VkSwapchainKHR swapchain;
		VkFormat imageFormat;
		VkExtent2D extent;
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;
	};

	inline static BoostrapSwapchainData boostrapSwapchain(
		uint32_t width,
		uint32_t height,
		VkPhysicalDevice physicalDevice,
		VkDevice logicalDevice,
		VkSurfaceKHR surface)
	{
		vkb::SwapchainBuilder swapchainBuilder{ physicalDevice, logicalDevice, surface };

		VkSurfaceFormatKHR desiredFormat
		{
			.format = VK_FORMAT_B8G8R8A8_SRGB,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};

		auto swapchain_ret = swapchainBuilder
			.set_desired_format(desiredFormat)
			.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
			.set_desired_min_image_count(3)
			.set_desired_extent(width, height)
			.build();

		vkb::Swapchain vkb_swapchain = swapchain_ret.value();
		VkSwapchainKHR swapchain = vkb_swapchain.swapchain;

		std::vector<VkImage> images = vkb_swapchain.get_images().value();
		std::vector<VkImageView> imageViews = vkb_swapchain.get_image_views().value();

		return BoostrapSwapchainData
		{
			.swapchain = swapchain,
			.imageFormat = vkb_swapchain.image_format,
			.extent = vkb_swapchain.extent,
			.images = images,
			.imageViews = imageViews
		};
	}
}