#pragma once

#include <spdlog/spdlog.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "../Engine/Camera.hpp"

namespace HelloVulkan
{
	struct UniformBufferObject {
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

	private:
		void InitWindow();
		void InitVulkan();
		void Loop();
		void Clean();

		void OnUpdate(float dt);
		void OnRender();

		void CreateSwapChain();
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

		void CreateTextureImage(const char* path);
		void CreateTextureImageView();
		void CreateTextureSampler();

		void LoadModel(const char* path);

		void CreateDepthResources();
		
		void DrawFrame();

		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		VkShaderModule CreateShaderModule(const std::vector<char>& code);
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		void CreateImage(CreateImageParams params);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void UpdateUniformBuffer(size_t currentImage);
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat FindDepthFormat();

		VkCommandBuffer StartTemporaryCommandBuffer();
		void EndTemporaryCommandBuffer(VkCommandBuffer commandBuffer);

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

		VkRenderPass _renderPass = VK_NULL_HANDLE;
		VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
		VkPipeline _graphicsPipeline = VK_NULL_HANDLE;

		std::vector<VkFramebuffer> _framebuffers;

		VkCommandPool _commandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> _commandBuffers;

		std::vector<VkSemaphore> _imageAvailableSemaphores;
		std::vector<VkSemaphore> _renderFinishedSemaphores;
		std::vector<VkFence> _inFlightFences;

		std::vector<Vertex> _vertices;
		std::vector<uint32_t> _indices;
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

		VkImage _textureImage = VK_NULL_HANDLE;
		VkImageView _textureImageView = VK_NULL_HANDLE;
		VkDeviceMemory _textureImageMemory = VK_NULL_HANDLE;
		VkSampler _textureSampler = VK_NULL_HANDLE;

		VkImage _depthImage = VK_NULL_HANDLE;
		VkImageView _depthImageView = VK_NULL_HANDLE;
		VkDeviceMemory _depthImageMemory = VK_NULL_HANDLE;

		size_t _currentFrame = 0;
		bool _framebufferResized = false;
				 
		uint32_t _width = WIDTH;
		uint32_t _height = HEIGHT;
		float _deltaTime = 0.0f;
		float _lastFrame = 0.0f;
		double _fps = 0.0f;
		Engine::Camera _camera;
	};
}