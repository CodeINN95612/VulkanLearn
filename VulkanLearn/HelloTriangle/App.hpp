#pragma once

#include <spdlog/spdlog.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace HelloTriangle
{
	class App
	{
	public:
		App();
		virtual ~App() {}

		void Run();

	public:
		static const uint32_t WIDTH = 800;
		static const uint32_t HEIGHT = 600;

	public:

	private:
		void InitWindow();
		void InitVulkan();
		void Loop();
		void Clean();

		void CreateInstance();
		void CreateMessenger();
		void DestroyMessenger();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSurface();
		void CreateSwapChain();

	private:
		GLFWwindow* _window = nullptr;
		VkInstance _instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT _messenger = VK_NULL_HANDLE;
		VkSurfaceKHR _surface = VK_NULL_HANDLE;

		VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
		VkDevice _logicalDevice = VK_NULL_HANDLE;
		VkQueue _graphicsQueue = VK_NULL_HANDLE;
		VkQueue _presentQueue = VK_NULL_HANDLE;

		VkSwapchainKHR _swapChain = VK_NULL_HANDLE;

	};
}