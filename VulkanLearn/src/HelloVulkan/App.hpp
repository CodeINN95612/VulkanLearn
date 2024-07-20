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

namespace HelloVulkan
{
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

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription GetBindingDescription() {
			VkVertexInputBindingDescription bindingDescription
			{
				.binding = 0,
				.stride = sizeof(Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			};

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

			return attributeDescriptions;
		}

		bool operator==(const Vertex& other) const {
			return pos == other.pos && color == other.color && texCoord == other.texCoord;
		}
	};

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
		static const uint32_t WIDTH = 1200;
		static const uint32_t HEIGHT = 800;
		static const size_t MAX_FRAMES_IN_FLIGHT = 2;

		const std::string MODEL_PATH = "assets/models/viking_room.obj";
		const std::string TEXTURE_PATH = "assets/textures/viking_room.png";

	public:
		inline void SetResized(bool resized, uint32_t width, uint32_t height);
		void OnScroll(double yoffset);

		inline const Engine::Camera& GetCamera() const { return _camera; }
		inline Engine::Camera& GetCamera() { return _camera; }

		inline Frame& Frame() { return _frames[_currentFrame]; }

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

		void LoadModel(const char* path);

		void DrawFrame();
		void DrawImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView);

		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void ClearBackground(VkCommandBuffer commandBuffer);

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

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
		bool _framebufferResized = false;
				 
		uint32_t _width = WIDTH;
		uint32_t _height = HEIGHT;
		float _deltaTime = 0.0f;
		float _lastFrame = 0.0f;
		double _fps = 0.0f;
		Engine::Camera _camera;

		Vulkan::Common::DeletionQueue DeletionQueue;
		VmaAllocator _allocator = nullptr;

		Image _drawImage = {};
		VkExtent2D _drawImageExtent = {};

		Vulkan::Common::DescriptorAllocator _descriptorAllocator = {};
		VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
		VkPipeline _pipeline = VK_NULL_HANDLE;
		VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

		VkFence _immFence;
		VkCommandBuffer _immCommandBuffer;
		VkCommandPool _immCommandPool;
	};
}