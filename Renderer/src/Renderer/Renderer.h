#pragma once

#include "Renderer/Defines.h"
#include "Renderer/Vulkan/Vulkan.h"
#include "Renderer/Shader.h"
#include "Renderer/Debug/Instrumentor.h"
#include "Renderer/CubeBuffer.h"

namespace vl::core
{
	typedef std::function<void()> ImGuiRenderFn;

	struct CameraUniformData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewProj;
		glm::vec4 cameraPos;
		float renderDistance;
	};

	struct Frame
	{
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;

		VkSemaphore ImageAvailableSemaphore;
		VkSemaphore RenderFinishedSemaphore;
		VkFence Fence;

		vulkan::AllocatedBuffer CubesStagingBuffer;
		vulkan::AllocatedBuffer CubesStorageBuffer;
		vulkan::AllocatedBuffer VisibleCubesStorageBuffer;
		vulkan::AllocatedBuffer IndirectDrawBuffer;
		vulkan::AllocatedBuffer CameraUniformBuffer;
		VkDescriptorSet CubesDescriptorSet;
	};

	class Renderer
	{
	public:
		Renderer(GLFWwindow* pWindow, uint32_t width, uint32_t height);
		virtual ~Renderer() = default;

		static std::unique_ptr<Renderer> Create(GLFWwindow* pWindow, uint32_t width, uint32_t height);

		CubeBuffer* GetCubeBuffer() { return &_cubes; }

		void OnResize(uint32_t width, uint32_t height);
		void OnImGuiRender(ImGuiRenderFn imguiRenderFuntion, bool showRendererWindow = true);

		void StartFrame(const glm::mat4& vpMatrix);
		void SubmitFrame(const CameraUniformData& cameraUniformData);

		void SetClearColor(glm::vec4 clearColor);

		void Init();
		void Shutdown();

	private:
		struct PushConstant
		{
			glm::mat4 transform;
		};

	private:
		const size_t CUBE_COUNT_PER_CHUNK = 16 * 16 * 16;
		const size_t CUBE_MAX_COUNT = CUBE_COUNT_PER_CHUNK * 32 * 32;
		const size_t CHUNK_SIZE_BYTES = CUBE_COUNT_PER_CHUNK * sizeof(CubeRenderData);
		const size_t BUFFER_MAX_SIZE_BYTES = CUBE_MAX_COUNT * sizeof(CubeRenderData);

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
		VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
		VkPipeline _graphicsPipeline = VK_NULL_HANDLE;
		VkPipeline _computePipeline = VK_NULL_HANDLE;

		Frame _frames[MAX_FRAMES_IN_FLIGHT] = {};

		VkDescriptorPool _imGuiDescriptorPool = VK_NULL_HANDLE;

		VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout _storageDescriptorSetLayout = VK_NULL_HANDLE;

		uint32_t _width = 0;
		uint32_t _height = 0;
		uint8_t _currentFrame = 0;
		bool _resized = false;
		bool _firstRender = true;
		glm::vec4 _clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		vulkan::DrawData _drawData = {};
		vulkan::ImmediateData _immediateData = {};

		VmaAllocator _allocator = nullptr;

		std::shared_ptr<vl::core::Shader> _vertexShader = nullptr;
		std::shared_ptr<vl::core::Shader> _fragmentShader = nullptr;
		std::shared_ptr<vl::core::Shader> _computeShader = nullptr;
		vulkan::AllocatedBuffer _vertexBuffer = {};
		vulkan::AllocatedBuffer _indexBuffer = {};

		glm::mat4 _vpMatrix = glm::mat4(1.0f);

		CubeBuffer _cubes = CubeBuffer(CUBE_MAX_COUNT);

	private:
		void InitVulkan();
		void InitSyncObjects();
		void InitSwapchain();
		void InitCommands();
		void InitGraphicsPipeline();
		void InitComputePipeline();
		void InitDescriptorPool();
		void InitImGui();

		void InitStorageDescriptors();
		void AllocateDescriptorSets(uint32_t count);
		void UpdateStorageDescriptorSet(VkDescriptorSet descriptorSet, const std::vector<VkBuffer>& bufferArray) const;

		void CreateSwapchain();
		void RecreateSwapchain();
		void DestroySwapchain();
		void DestroyImgui();

		void ExecuteComputeShader();

		void DrawFrame(const CameraUniformData& cameraUniformData);
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void DrawBackground(VkCommandBuffer commandBuffer);
		void DrawGeometry(VkCommandBuffer commandBuffer);
		void DrawImGui(VkCommandBuffer commandBuffer, VkImageView targetImageView);

		void UploadMesh();
		void GenerateTransformsStorageBuffer();
		void UpdateTransformsStorage() const;
		void UpdateCameraUniformBuffer(const CameraUniformData& cameraUniformData) const;

		vulkan::AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const;

		inline Frame& CurrentFrame() { return _frames[_currentFrame]; }
		inline const Frame& CurrentFrame() const { return _frames[_currentFrame]; }
	};
}