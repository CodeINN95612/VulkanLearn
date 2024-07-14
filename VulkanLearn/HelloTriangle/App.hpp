#pragma once

#include <spdlog/spdlog.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "../src/Camera.hpp"

namespace HelloTriangle
{
	struct CreateImageParams
	{
		uint32_t width;
		uint32_t height;
		VkFormat format;
		VkImageTiling tiling;
		VkImageUsageFlags usage;
		VkMemoryPropertyFlags properties;
		VkImage& image;
		VkDeviceMemory& imageMemory;
	};

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
		void OnScroll(double yoffset);

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

		void CreateDescriptorSetLayout();
		void CreateUniformBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSets();

		void CreateTextureImage();
		
		void DrawFrame();

		VkShaderModule CreateShaderModule(const std::vector<char>& code);
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		void CreateImage(CreateImageParams params);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void UpdateUniformBuffer(size_t currentImage);

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

		VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;

		std::vector<VkDescriptorSet> _descriptorSets;
		std::vector<VkBuffer> _uniformBuffers;
		std::vector<VkDeviceMemory> _uniformBuffersMemory;
		std::vector<void*> _uniformBuffersMapped;

		VkImage _textureImage;
		VkDeviceMemory _textureImageMemory;

		size_t _currentFrame = 0;
		bool _framebufferResized = false;

		Camera _camera;
	};
}