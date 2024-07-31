#pragma once

#include "Renderer/Defines.h"
#include "Renderer/Vulkan/Vulkan.h"
#include "Renderer/Shader.h"

namespace vl::core
{
	typedef std::function<void()> ImGuiRenderFn;

	class Renderer
	{
	public:
		Renderer(GLFWwindow* pWindow, uint32_t width, uint32_t height);
		virtual ~Renderer() = default;

		static std::unique_ptr<Renderer> Create(GLFWwindow* pWindow, uint32_t width, uint32_t height);

		void OnResize(uint32_t width, uint32_t height);
		void OnImGuiRender(ImGuiRenderFn imguiRenderFuntion);

		void StartFrame();
		void SubmitFrame();

		void SetClearColor(glm::vec4 clearColor);
		void DrawTriangle();

		void Init();
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
		VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
		VkPipeline _graphicsPipeline = VK_NULL_HANDLE;

		VkDescriptorPool _imGuiDescriptorPool = VK_NULL_HANDLE;

		uint32_t _width = 0;
		uint32_t _height = 0;
		uint8_t _currentFrame = 0;
		bool _resized = false;
		glm::vec4 _clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		vulkan::DrawData _drawData = {};
		vulkan::ImmediateData _immediateData = {};

		VmaAllocator _allocator = nullptr;

		std::shared_ptr<vl::core::Shader> _vertexShader = nullptr;
		std::shared_ptr<vl::core::Shader> _fragmentShader = nullptr;

	private:
		void InitVulkan();
		void InitSyncObjects();
		void InitSwapchain();
		void InitCommands();
		void InitGraphicsPipeline();
		void InitImGui();

		void CreateSwapchain();
		void RecreateSwapchain();
		void DestroySwapchain();
		void DestroyImgui();

		void DrawFrame();
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void DrawBackground(VkCommandBuffer commandBuffer);
		void DrawGeometry(VkCommandBuffer commandBuffer);
		void DrawImGui(VkCommandBuffer commandBuffer, VkImageView targetImageView);

		inline vulkan::Frame& CurrentFrame() { return _frames[_currentFrame]; }
	};
}