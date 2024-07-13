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
		static const size_t MAX_FRAMES_IN_FLIGHT = 2;

	public:
		inline void SetResized(bool resized) { _framebufferResized = resized; }

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
		void CreateImageViews();
		void CreateRenderPass();
		void CreateGraphicsPipeline();
		void CreateFramebuffers();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void RecreateSwapChain();
		void CleanUpSwapChain();

		void CreateVertexBuffer();
		void CreateIndexBuffer();
		
		void DrawFrame();

		VkShaderModule CreateShaderModule(const std::vector<char>& code);
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

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
		VkFormat _swapChainImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D _swapChainExtent = {};
		std::vector<VkImage> _swapChainImages;
		std::vector<VkImageView> _swapChainImageViews;

		VkRenderPass _renderPass = VK_NULL_HANDLE;
		VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
		VkPipeline _graphicsPipeline = VK_NULL_HANDLE;

		std::vector<VkFramebuffer> _framebuffers;

		VkCommandPool _commandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> _commandBuffers;

		std::vector<VkSemaphore> _imageAvailableSemaphores;
		std::vector<VkSemaphore> _renderFinishedSemaphores;
		std::vector<VkFence> _inFlightFences;

		VkBuffer _vertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory  _vertexBufferMemory = VK_NULL_HANDLE;
		VkBuffer _indexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory _indexBufferMemory = VK_NULL_HANDLE;

		size_t _currentFrame = 0;
		bool _framebufferResized = false;
	};
}