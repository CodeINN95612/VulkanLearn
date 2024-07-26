#pragma once

#include "Renderer/Defines.h"
#include "Renderer/Vulkan/Vulkan.h"

namespace vl::core
{
	class Renderer
	{
	public:
		Renderer(GLFWwindow* pWindow, uint32_t width, uint32_t height);
		virtual ~Renderer() = default;

		static std::unique_ptr<Renderer> Create(GLFWwindow* pWindow, uint32_t width, uint32_t height);

		void OnResize(uint32_t width, uint32_t height);
		void OnRenderFinished();

		void Init();
		void Render();
		void Shutdown();

	private:
		GLFWwindow* _pWindow;
		VkInstance _instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR _surface = VK_NULL_HANDLE;
		VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
		VkDevice _logicalDevice = VK_NULL_HANDLE;
		VkQueue _graphicsQueue = VK_NULL_HANDLE;
		uint32_t _graphicsQueueFamilyIndex = 0;
		VkQueue _presentQueue = VK_NULL_HANDLE;
		uint32_t _presentQueueFamilyIndex = 0;
		vulkan::Swapchain _swapchain = {};
		vulkan::Frame _frames[MAX_FRAMES_IN_FLIGHT] = {};

		uint32_t _width = 0;
		uint32_t _height = 0;
		uint8_t _currentFrame = 0;
		bool _resized = false;

		vulkan::DrawData _drawData = {};
		vulkan::ImmediateData _immediateData = {};

		VmaAllocator _allocator = nullptr;

	private:
		void InitVulkan();
		void InitSyncObjects();
		void InitSwapchain();
		void InitCommands();
		void InitDescriptors();
		void InitComputePipeline();
		void InitGraphicsPipeline();

		void CreateSwapchain();
		void DestroySwapchain();

		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void DrawBackground(VkCommandBuffer commandBuffer);
		void DrawGeometry(VkCommandBuffer commandBuffer);

		inline vulkan::Frame& CurrentFrame() { return _frames[_currentFrame]; }
	};
}