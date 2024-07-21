#pragma once

#include <spdlog/spdlog.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vma/vk_mem_alloc.h>

#include "../Engine/Camera.hpp"
#include "../Vulkan/Common/DeletionQueue.hpp"
#include "../Vulkan/Common/DescriptorAllocator.hpp"
#include "../Vulkan/Common/DescriptorLayoutBuilder.hpp"
#include "../Vulkan/Pipeline.hpp"
#include "../Vulkan/Loader.hpp"

namespace HelloVulkan
{
	struct ComputePushConstants
	{
		glm::vec4 Data1;
		glm::vec4 Data2;
		glm::vec4 Data3;
		glm::vec4 Data4;
	};

	struct ComputeEffect {
		const char* Name;

		VkPipeline Pipeline;
		VkPipelineLayout Layout;

		ComputePushConstants Data;
	};

	struct Image
	{
		VkImage Image;
		VkImageView ImageView;
		VmaAllocation Allocation;
		VkExtent3D ImageExtent;
		VkFormat ImageFormat;
	};

	struct Frame 
	{
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;

		VkSemaphore ImageAvailableSemaphore;
		VkSemaphore RenderFinishedSemaphore;
		VkFence Fence;

		Vulkan::Common::DeletionQueue DeletionQueue;
	};

	struct UniformBufferObject 
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	class App
	{
	public:
		App();
		virtual ~App() {}

		void Run();

	public:
		static const uint32_t WIDTH = 1200;
		static const uint32_t HEIGHT = 800;
		static const size_t MAX_FRAMES_IN_FLIGHT = 2;

		const std::string MODEL_PATH = "assets/models/viking_room.obj";
		const std::string TEXTURE_PATH = "assets/textures/viking_room.png";

	public:
		inline void SetResized(bool resized, uint32_t width, uint32_t height);
		void OnScroll(double yoffset);

		inline Frame& Frame() { return _frames[_currentFrame]; }

		Vulkan::GPUMeshBuffers UploadMesh(std::span<uint32_t> indices, std::span<Vulkan::Vertex> vertices);

	private:
		void InitWindow();
		void InitVulkan();
		void Loop();
		void Clean();

		void OnUpdate(float dt);
		void OnImGuiRender();
		void OnRender();

		void CreateSwapChain();
		void CreateCommands();
		void CreateSyncObjects();
		void RecreateSwapChain();
		void CleanUpSwapChain();
		void CreateDescriptors();
		void CreatePipeline();
		void InitializeImgui();
		void CreateMeshPipeline();

		void DrawFrame();
		void DrawImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView);

		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void DrawBackground(VkCommandBuffer commandBuffer);
		void DrawGeometry(VkCommandBuffer commandBuffer);

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

		Vulkan::AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void DestroyBuffer(const Vulkan::AllocatedBuffer& buffer);
		void UploadDefaultMeshData();

	private:
		bool _doRender = true;

		GLFWwindow* _window = nullptr;
		VkInstance _instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT _messenger = VK_NULL_HANDLE;
		VkSurfaceKHR _surface = VK_NULL_HANDLE;

		VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
		VkDevice _logicalDevice = VK_NULL_HANDLE;
		VkQueue _graphicsQueue = VK_NULL_HANDLE;
		uint32_t _graphicsQueueFamilyIndex = 0;
		VkQueue _presentQueue = VK_NULL_HANDLE;
		uint32_t _presentQueueFamilyIndex = 0;

		VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
		VkFormat _swapChainImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D _swapChainExtent = {};
		std::vector<VkImage> _swapChainImages;
		std::vector<VkImageView> _swapChainImageViews;

		HelloVulkan::Frame _frames[MAX_FRAMES_IN_FLIGHT];
		size_t _currentFrame = 0;
		bool _resized = false;
				 
		uint32_t _width = WIDTH;
		uint32_t _height = HEIGHT;
		float _deltaTime = 0.0f;
		float _lastFrame = 0.0f;
		double _fps = 0.0f;

		Vulkan::Common::DeletionQueue DeletionQueue;
		VmaAllocator _allocator = nullptr;

		Image _drawImage = {};
		VkExtent2D _drawImageExtent = {};

		Image _depthImage;

		Vulkan::Common::DescriptorAllocator _descriptorAllocator = {};
		VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;

		std::vector<ComputeEffect> _backgroundEffects;
		int _currentBackgroundEffect = 0;

		VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

		VkFence _immFence;
		VkCommandBuffer _immCommandBuffer;
		VkCommandPool _immCommandPool;

		VkPipelineLayout _meshPipelineLayout;
		VkPipeline _meshPipeline;

		std::vector<std::shared_ptr<Vulkan::Loader::MeshAsset>> _testMeshes;
		int _currentMesh = 2;
	};
}