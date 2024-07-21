#include "App.hpp"

#include <set>
#include <fstream>
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/hash.hpp>

#include <stb/stb_image.h>

#include <tinyobjloader/tiny_obj_loader.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/vulkan/imgui_impl_vulkan.h>

#include "../Engine/InputState.hpp"
#include "../Vulkan/Utils.hpp"
#include "../Vulkan/Init.hpp"
#include "../Vulkan/Image.hpp"
#include "../Common/Utils.hpp"
#include "../Engine/Model.hpp"
#include "../Vulkan/Common/GraphicsPipelineBuilder.hpp"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback( 
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
	VkDebugUtilsMessageTypeFlagsEXT messageType, 
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
	void* pUserData) {
	
	const char* type = "Unknown";
	switch (messageType) 
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: 
			type = "GENERAL";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			type = "VALIDACION";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: 
			type = "RENDIMIENTO";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: 
			type = "DEVICE ADDRESS BINDING";
			break;
	}

	switch (messageSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			spdlog::debug("[VULKAN] Tipo: {0}, Mensaje: {1}", type, pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			spdlog::info("[VULKAN] Tipo: {0}, Mensaje: {1}", type, pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			spdlog::warn("[VULKAN] Tipo: {0}, Mensaje: {1}", type, pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			spdlog::error("[VULKAN] Tipo: {0}, Mensaje: {1}", type, pCallbackData->pMessage);
			break;
	}

	return VK_FALSE;
}

namespace HelloVulkan
{
	App::App()
	{
	}

	void App::Run()
	{
		InitWindow();
		InitVulkan();
		Loop();
		Clean();
	}

	inline void App::SetResized(bool resized, uint32_t width, uint32_t height)
	{
		_resized = resized;
		_width = width;
		_height = height;

		bool minimized = width == 0 || height == 0;
		_doRender = !minimized;
	}

	void App::OnScroll(double yoffset)
	{
	}

	void App::InitWindow()
	{
		spdlog::info("Inicializando Ventana");

		if (glfwInit() == GLFW_FALSE)
		{
			throw std::exception("Error al inicializar glfw");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		_window = glfwCreateWindow(_width, _height, "Vulkan Triangle", nullptr, nullptr);
		//glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetWindowUserPointer(_window, this);

		glfwSetFramebufferSizeCallback(_window, [](GLFWwindow* window, int width, int height)
			{
				auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
				app->SetResized(true, width, height);
			}
		);

		glfwSetScrollCallback(_window, [](GLFWwindow* window, double xoffset, double yoffset)
			{
				auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
				app->OnScroll(yoffset);
			}
		);

		if (!_window) {
			throw std::exception("Error al crear la ventana");
		}

		spdlog::info("Inicializacion de Ventana exitosa");
	}

    void App::Loop()
    {
        double lastTime = glfwGetTime();
        int frameCount = 0;

        while (!glfwWindowShouldClose(_window)) {
            float currentFrame = static_cast<float>(glfwGetTime());
            _deltaTime = currentFrame - _lastFrame;
            _lastFrame = currentFrame;

            glfwPollEvents();

			OnUpdate(_deltaTime);


			if (_doRender)
			{
				OnImGuiRender();
				OnRender();
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

            // Calculate FPS
            double currentTime = glfwGetTime();
            frameCount++;
            if (currentTime - lastTime >= 0.5) {
				_fps = frameCount / (currentTime - lastTime);;
                frameCount = 0;
                lastTime = currentTime;
            }
        }

        VK_CHECK(vkDeviceWaitIdle(_logicalDevice));
    }

	void App::Clean()
	{
		spdlog::info("Limpiando");

		vkDestroyImageView(_logicalDevice, _depthImage.ImageView, nullptr);
		vmaDestroyImage(_allocator, _depthImage.Image, _depthImage.Allocation);

		vkDestroyImageView(_logicalDevice, _drawImage.ImageView, nullptr);
		vmaDestroyImage(_allocator, _drawImage.Image, _drawImage.Allocation);

		DeletionQueue.Flush();

		vkDestroyDescriptorSetLayout(_logicalDevice, _descriptorSetLayout, nullptr);
		_descriptorAllocator.DestroyPool(_logicalDevice);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(_logicalDevice, _frames[i].ImageAvailableSemaphore, nullptr);
			vkDestroySemaphore(_logicalDevice, _frames[i].RenderFinishedSemaphore, nullptr);
			vkDestroyFence(_logicalDevice, _frames[i].Fence, nullptr);
			vkDestroyCommandPool(_logicalDevice, _frames[i].CommandPool, nullptr);

			_frames[i].DeletionQueue.Flush();
		}

		CleanUpSwapChain();

		Vulkan::BoostrapData data
		{
			.instance = _instance,
			.debugMessenger = _messenger,
			.surface = _surface,
			.physicalDevice = _physicalDevice,
			.logicalDevice = _logicalDevice,
			.graphicsQueue = _graphicsQueue,
			.graphicsQueueFamilyIndex = _graphicsQueueFamilyIndex,
			.presentQueue = _presentQueue,
			.presentQueueFamilyIndex = _presentQueueFamilyIndex
		};
		Vulkan::cleanBoostrapedData(data);

		glfwDestroyWindow(_window);
		glfwTerminate();
	}

	void App::OnUpdate(float dt)
	{
		if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(_window, GLFW_TRUE);
		}
	}

	void App::OnImGuiRender()
	{
		if (_resized) {
			RecreateSwapChain();
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		static bool open = true;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		ImGuiWindowFlags window_flags = /*ImGuiWindowFlags_MenuBar |*/ ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("DockSpace Demo", &open, window_flags);

		ImGui::PopStyleVar();

		ImGui::PopStyleVar(2);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (ImGui::Begin("Utils"))
		{
			ImGui::Text("FPS: %.1f", _fps);

			ImGui::Text("Width: %d", _width);
			ImGui::Text("Height: %d", _height);
		}
		ImGui::End();

		if (ImGui::Begin("Background")) {

			ComputeEffect& selected = _backgroundEffects[_currentBackgroundEffect];

			ImGui::Text("Selected effect: ", selected.Name);

			ImGui::SliderInt("Effect Index", &_currentBackgroundEffect, 0, uint32_t(_backgroundEffects.size() - 1));

			ImGui::ColorEdit4("Data1", (float*)&selected.Data.Data1);
			ImGui::ColorEdit4("Data2", (float*)&selected.Data.Data2);
			ImGui::ColorEdit4("Data3", (float*)&selected.Data.Data3);
			ImGui::ColorEdit4("Data4", (float*)&selected.Data.Data4);
		}
		ImGui::End();

		if (ImGui::Begin("Mesh")) {
			ImGui::Text("Selected mesh: ", _testMeshes[_currentMesh]->Name);

			ImGui::SliderInt("Index", &_currentMesh, 0, uint32_t(_testMeshes.size() - 1));
		}
		ImGui::End();

		//ImGui::ShowDemoWindow();

		ImGui::End(); //Dockspace

		ImGui::Render();
	}

	void App::OnRender()
	{
		DrawFrame();
	}

	void App::InitVulkan()
	{
		auto data = Vulkan::boostrapVulkan(_window, messageCallback, true);
		_instance = data.instance;
		_messenger = data.debugMessenger;
		_surface = data.surface;
		_physicalDevice = data.physicalDevice;
		_logicalDevice = data.logicalDevice;
		_graphicsQueue = data.graphicsQueue;
		_graphicsQueueFamilyIndex = data.graphicsQueueFamilyIndex;
		_presentQueue = data.presentQueue;
		_presentQueueFamilyIndex = data.presentQueueFamilyIndex;

		VmaAllocatorCreateInfo allocatorInfo
		{
			.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			.physicalDevice = _physicalDevice,
			.device = _logicalDevice,
			.instance = _instance,
		};
		vmaCreateAllocator(&allocatorInfo, &_allocator);

		DeletionQueue.Push([&]() 
			{
				vmaDestroyAllocator(_allocator);
			}
		);

		CreateSyncObjects();
		CreateSwapChain();
		CreateCommands();
		CreateDescriptors();
		CreatePipeline();
		InitializeImgui();
		CreateMeshPipeline();
		UploadDefaultMeshData();
	}

	void App::CreateSwapChain()
	{
		auto data = Vulkan::boostrapSwapchain(_width, _height, _physicalDevice, _logicalDevice, _surface);
		_swapChain = data.swapchain;
		_swapChainImageFormat = data.imageFormat;
		_swapChainExtent = data.extent;
		_swapChainImages = data.images;
		_swapChainImageViews = data.imageViews;

		//Draw Image
		if (_drawImage.Image != VK_NULL_HANDLE)
		{
			vkDestroyImageView(_logicalDevice, _drawImage.ImageView, nullptr);
			vmaDestroyImage(_allocator, _drawImage.Image, _drawImage.Allocation);
			_drawImage.Image = VK_NULL_HANDLE;
		}

		VkExtent3D drawImageExtent
		{
			.width = _width,
			.height = _height,
			.depth = 1,
		};

		_drawImage.ImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		_drawImage.ImageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
			| VK_IMAGE_USAGE_TRANSFER_DST_BIT
			| VK_IMAGE_USAGE_STORAGE_BIT
			| VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo drawImageInfo = Vulkan::Init::imageCreateInfo(
			_drawImage.ImageFormat,
			drawImageUsages,
			_drawImage.ImageExtent);

		VmaAllocationCreateInfo imageAllocInfo
		{
			.usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		};

		VK_CHECK(vmaCreateImage(_allocator, &drawImageInfo, &imageAllocInfo, &_drawImage.Image, &_drawImage.Allocation, nullptr));

		VkImageViewCreateInfo drawImageViewInfo = Vulkan::Init::imageViewCreateInfo(
			_drawImage.ImageFormat,
			_drawImage.Image,
			VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(_logicalDevice, &drawImageViewInfo, nullptr, &_drawImage.ImageView));

		//Depth Image
		if (_depthImage.Image != VK_NULL_HANDLE)
		{
			vkDestroyImageView(_logicalDevice, _depthImage.ImageView, nullptr);
			vmaDestroyImage(_allocator, _depthImage.Image, _depthImage.Allocation);
			_depthImage.Image = VK_NULL_HANDLE;
		}

		_depthImage.ImageFormat = VK_FORMAT_D32_SFLOAT;
		_depthImage.ImageExtent = drawImageExtent;
		VkImageUsageFlags depthImageUsages = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkImageCreateInfo depthImageInfo = Vulkan::Init::imageCreateInfo(_depthImage.ImageFormat, depthImageUsages, drawImageExtent);

		vmaCreateImage(_allocator, &depthImageInfo, &imageAllocInfo, &_depthImage.Image, &_depthImage.Allocation, nullptr);

		VkImageViewCreateInfo depthImageViewInfo = Vulkan::Init::imageViewCreateInfo(
			_depthImage.ImageFormat, 
			_depthImage.Image, 
			VK_IMAGE_ASPECT_DEPTH_BIT);

		VK_CHECK(vkCreateImageView(_logicalDevice, &depthImageViewInfo, nullptr, &_depthImage.ImageView));
	}

	void App::CreateCommands()
	{
		VkCommandPoolCreateInfo poolInfo = Vulkan::Init::commandPoolCreateInfo(
			_graphicsQueueFamilyIndex,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VK_CHECK(vkCreateCommandPool(_logicalDevice, &poolInfo, nullptr, &_frames[i].CommandPool));

			VkCommandBufferAllocateInfo allocInfo = Vulkan::Init::commandBufferAllocateInfo(_frames[i].CommandPool, 1);
			VK_CHECK(vkAllocateCommandBuffers(_logicalDevice, &allocInfo, &_frames[i].CommandBuffer));
		}
		
		// Immediate ImGui Command Pool
		VK_CHECK(vkCreateCommandPool(_logicalDevice, &poolInfo, nullptr, &_immCommandPool));
		
		VkCommandBufferAllocateInfo immAllocInfo = Vulkan::Init::commandBufferAllocateInfo(_immCommandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(_logicalDevice, &immAllocInfo, &_immCommandBuffer));

		DeletionQueue.Push([=]()
			{
				vkDestroyCommandPool(_logicalDevice, _immCommandPool, nullptr);
			}
		);

	}

	void App::CreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo = Vulkan::Init::semaphoreCreateInfo();
		VkFenceCreateInfo fenceInfo = Vulkan::Init::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VK_CHECK(vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_frames[i].ImageAvailableSemaphore));

			VK_CHECK(vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_frames[i].RenderFinishedSemaphore));

			VK_CHECK(vkCreateFence(_logicalDevice, &fenceInfo, nullptr, &_frames[i].Fence));
		}

		// Immediate ImGui Sync Objects
		VK_CHECK(vkCreateFence(_logicalDevice, &fenceInfo, nullptr, &_immFence));

		DeletionQueue.Push([=]()
			{
				vkDestroyFence(_logicalDevice, _immFence, nullptr);
			}
		);
	}

	void App::RecreateSwapChain()
	{
		VK_CHECK(vkDeviceWaitIdle(_logicalDevice));

		CleanUpSwapChain();

		CreateSwapChain();

		vkDestroyDescriptorSetLayout(_logicalDevice, _descriptorSetLayout, nullptr);
		_descriptorAllocator.DestroyPool(_logicalDevice);
		CreateDescriptors();

		_resized = false;
	}

	void App::CleanUpSwapChain()
	{
		vkDestroySwapchainKHR(_logicalDevice, _swapChain, nullptr);

		for (auto imageView : _swapChainImageViews) {
			vkDestroyImageView(_logicalDevice, imageView, nullptr);
		}
	}

	void App::CreateDescriptors()
	{
		Vulkan::Common::DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		_descriptorSetLayout = builder.Build(_logicalDevice, VK_SHADER_STAGE_COMPUTE_BIT);

		std::vector<Vulkan::Common::DescriptorAllocator::PoolSizeRatio> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
		};

		_descriptorAllocator.InitPool(_logicalDevice, 10, sizes);

		_descriptorSet = _descriptorAllocator.Allocate(_logicalDevice, _descriptorSetLayout);

		VkDescriptorImageInfo imageInfo
		{
			.imageView = _drawImage.ImageView,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		};

		VkWriteDescriptorSet writeDescriptorSet
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = _descriptorSet,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = &imageInfo,
		};

		vkUpdateDescriptorSets(_logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
	}

	void App::CreatePipeline()
	{

		VkPushConstantRange pushConstant
		{
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			.offset = 0,
			.size = sizeof(ComputePushConstants),
		};

		VkPipelineLayoutCreateInfo computeLayout
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.setLayoutCount = 1,
			.pSetLayouts = &_descriptorSetLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstant,
		};

		VK_CHECK(vkCreatePipelineLayout(_logicalDevice, &computeLayout, nullptr, &_pipelineLayout));

		VkShaderModule computeShaderModule = Vulkan::Pipeline::loadShaderModule(_logicalDevice, "assets/shaders/shader.comp.spv");	
		VkPipelineShaderStageCreateInfo shaderStageInfo
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = computeShaderModule,
			.pName = "main",
		};

		VkComputePipelineCreateInfo pipelineInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = shaderStageInfo,
			.layout = _pipelineLayout,
		};

		ComputeEffect gradient
		{
			.Name = "gradient",
			.Layout = _pipelineLayout,
			.Data
			{
				.Data1 = glm::vec4(0.007f, 0.007f, 0.007f, 1),
				.Data2 = glm::vec4(0.005f, 0.007f, 0.007f, 1),
			}
		};

		VK_CHECK(vkCreateComputePipelines(_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gradient.Pipeline));

		vkDestroyShaderModule(_logicalDevice, computeShaderModule, nullptr);

		_backgroundEffects.push_back(gradient);

		DeletionQueue.Push([=]()
			{
				vkDestroyPipeline(_logicalDevice, gradient.Pipeline, nullptr);
				vkDestroyPipelineLayout(_logicalDevice, _pipelineLayout, nullptr);
			}
		);
	}

	void App::InitializeImgui()
	{
		VkDescriptorPoolSize poolSizes[] = 
		{ 
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } 
		};

		VkDescriptorPoolCreateInfo poolInfo
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets = 1000,
			.poolSizeCount = (uint32_t)std::size(poolSizes),
			.pPoolSizes = poolSizes,
		};

		VkDescriptorPool imguiPool;
		VK_CHECK(vkCreateDescriptorPool(_logicalDevice, &poolInfo, nullptr, &imguiPool));

		ImGui::CreateContext();
		bool initialized = ImGui_ImplGlfw_InitForVulkan(_window, true);
		if (!initialized)
		{
			throw std::exception("No fue posible inicializar la implementacion ImGui para Vulkan");
		}

		VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &_swapChainImageFormat,
		};

		ImGui_ImplVulkan_InitInfo initInfo
		{
			.Instance = _instance,
			.PhysicalDevice = _physicalDevice,
			.Device = _logicalDevice,
			.QueueFamily = _graphicsQueueFamilyIndex,
			.Queue = _graphicsQueue,
			.DescriptorPool = imguiPool,
			.MinImageCount = 3,
			.ImageCount = 3,
			.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
			.UseDynamicRendering = true,
			.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
		};

		auto& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		
		initialized = ImGui_ImplVulkan_Init(&initInfo);
		if (!initialized)
		{
			throw std::exception("No fue posible inicializar ImGui para Vulkan");
		}

		ImGui_ImplVulkan_CreateFontsTexture();

		DeletionQueue.Push([=]()
			{
				ImGui_ImplVulkan_Shutdown();
				ImGui_ImplGlfw_Shutdown();
				ImGui::DestroyContext();
				vkDestroyDescriptorPool(_logicalDevice, imguiPool, nullptr);
			}
		);
	}

	void App::CreateMeshPipeline()
	{
		VkShaderModule vertexShader = Vulkan::Pipeline::loadShaderModule(_logicalDevice, "assets/shaders/triangle.vert.spv");
		VkShaderModule fragmentShader = Vulkan::Pipeline::loadShaderModule(_logicalDevice, "assets/shaders/triangle.frag.spv");

		auto size = sizeof(Vulkan::GPUDrawPushConstants);
		VkPushConstantRange bufferRange
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(Vulkan::GPUDrawPushConstants),
		};

		VkPipelineLayoutCreateInfo layoutInfo = Vulkan::Init::pipelineLayoutCreateInfo();
		layoutInfo.pPushConstantRanges = &bufferRange;
		layoutInfo.pushConstantRangeCount = 1;

		VK_CHECK(vkCreatePipelineLayout(_logicalDevice, &layoutInfo, nullptr, &_meshPipelineLayout));

		Vulkan::Common::GraphicsPipelineBuilder pipelineBuilder(_meshPipelineLayout);

		_meshPipeline = pipelineBuilder
			.SetShaders(vertexShader, fragmentShader)
			.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			.SetPolygonMode(VK_POLYGON_MODE_FILL)
			.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
			.SetMultisamplingNone()
			.enableBlendingAlphablend()
			.EnableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
			.SetColorAttachmentFormat(_drawImage.ImageFormat)
			.SetDepthFormat(_depthImage.ImageFormat)
			.Build(_logicalDevice);

		vkDestroyShaderModule(_logicalDevice, vertexShader, nullptr);
		vkDestroyShaderModule(_logicalDevice, fragmentShader, nullptr);

		DeletionQueue.Push([=]()
			{
				vkDestroyPipeline(_logicalDevice, _meshPipeline, nullptr);
				vkDestroyPipelineLayout(_logicalDevice, _meshPipelineLayout, nullptr);
			}
		);
	}

	void App::DrawFrame()
	{
		VK_CHECK(vkWaitForFences(_logicalDevice, 1, &Frame().Fence, VK_TRUE, UINT64_MAX));

		Frame().DeletionQueue.Flush();

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(_logicalDevice, _swapChain, UINT64_MAX, Frame().ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			_resized = true;
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::exception("No fue posible obtener la imagen");
		}

		VK_CHECK(vkResetFences(_logicalDevice, 1, &Frame().Fence));

		_drawImageExtent.width = std::min(_swapChainExtent.width, _drawImage.ImageExtent.width);
		_drawImageExtent.height = std::min(_swapChainExtent.height, _drawImage.ImageExtent.height);

		VkCommandBuffer currentCommandBuffer = Frame().CommandBuffer;

		VK_CHECK(vkResetCommandBuffer(currentCommandBuffer, 0));

		RecordCommandBuffer(currentCommandBuffer, imageIndex);

		VkCommandBufferSubmitInfo commandBufferInfo = Vulkan::Init::commandBufferSubmitInfo(currentCommandBuffer);

		VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo = Vulkan::Init::semaphoreSubmitInfo(
			Frame().ImageAvailableSemaphore, 
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);

		VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo = Vulkan::Init::semaphoreSubmitInfo(
			Frame().RenderFinishedSemaphore,
			VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

		VkSubmitInfo2 submitInfo2 = Vulkan::Init::submitInfo2(commandBufferInfo, &waitSemaphoreSubmitInfo,  &signalSemaphoreSubmitInfo);

		VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submitInfo2, Frame().Fence));

		VkPresentInfoKHR presentInfo = Vulkan::Init::presentInfoKHR(&_swapChain, &imageIndex, &Frame().RenderFinishedSemaphore);

		result = vkQueuePresentKHR(_presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			_resized = true;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::exception("No fue posible presentar en la cola de presentacion");
		}

		_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void App::DrawImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView)
	{
		VkRenderingAttachmentInfo colorAttachment = Vulkan::Init::colorAttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderInfo = Vulkan::Init::renderingInfo(_swapChainExtent, &colorAttachment, nullptr);

		vkCmdBeginRendering(commandBuffer, &renderInfo);

		ImDrawData* data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(data, commandBuffer);

		vkCmdEndRendering(commandBuffer);
	}

	void App::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
			.pInheritanceInfo = nullptr,
		};

		VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		Vulkan::Image::transitionImage(commandBuffer, _drawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		DrawBackground(commandBuffer);

		Vulkan::Image::transitionImage(commandBuffer, _drawImage.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		Vulkan::Image::transitionImage(commandBuffer, _depthImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		DrawGeometry(commandBuffer);

		Vulkan::Image::transitionImage(commandBuffer, _drawImage.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		Vulkan::Image::transitionImage(commandBuffer, _swapChainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		Vulkan::Image::copyImage(commandBuffer, _drawImage.Image, _swapChainImages[imageIndex], _drawImageExtent, _swapChainExtent);

		Vulkan::Image::transitionImage(commandBuffer, _swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		DrawImgui(commandBuffer, _swapChainImageViews[imageIndex]);

		Vulkan::Image::transitionImage(commandBuffer, _swapChainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		VK_CHECK(vkEndCommandBuffer(commandBuffer));
	}

	void App::DrawBackground(VkCommandBuffer commandBuffer)
	{
		VkClearColorValue clearColor = { {0.007f, 0.007f, 0.007f, 1.0f} };
		VkImageSubresourceRange clearRange = Vulkan::Init::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

		vkCmdClearColorImage(commandBuffer, _drawImage.Image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);

		/*ComputeEffect& effect = _backgroundEffects[_currentBackgroundEffect];

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, effect.Pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);

		vkCmdPushConstants(commandBuffer, _pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.Data);

		vkCmdDispatch(commandBuffer, _drawImageExtent.width / 16, _drawImageExtent.height / 16, 1);*/
	}

	void App::DrawGeometry(VkCommandBuffer commandBuffer)
	{
		VkRenderingAttachmentInfo colorAttachment = Vulkan::Init::colorAttachmentInfo(
			_drawImage.ImageView, 
			nullptr, 
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingAttachmentInfo depthAttachment = Vulkan::Init::depthAttachmentInfo(
			_depthImage.ImageView,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = Vulkan::Init::renderingInfo(
			_drawImageExtent, 
			&colorAttachment, 
			&depthAttachment);

		vkCmdBeginRendering(commandBuffer, &renderInfo);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);

		VkViewport viewport
		{
			.x = 0,
			.y = 0,
			.width = float(_drawImageExtent.width),
			.height = float(_drawImageExtent.height),
			.minDepth = 1.f,
			.maxDepth = 0.f,
		};

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor
		{
			.offset
			{
				.x = 0,
				.y = 0
			},
			.extent
			{
				.width = _drawImageExtent.width,
				.height = _drawImageExtent.height,
			}
		};

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);



		glm::mat4 view = glm::lookAt(glm::vec3{ 0, 5, 10 }, glm::vec3{ 0, 0, 0 }, glm::vec3{ 0, 1, 0 });
		glm::mat4 projection = glm::perspective(glm::radians(45.f), (float)_drawImageExtent.width / (float)_drawImageExtent.height, 0.1f, 100.f);
		projection[1][1] *= -1;

		static float rotation = 0.0f;
		glm::mat4 model = projection * glm::rotate(view, glm::radians(rotation), glm::vec3(0, 1, 0));
		rotation += 0.1f;

		Vulkan::GPUDrawPushConstants pushConstants
		{
			.ModelMatrix = model,
			.VertexBuffer = _testMeshes[_currentMesh]->MeshBuffers.VertexBufferAddress
		};

		vkCmdPushConstants(commandBuffer, _meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Vulkan::GPUDrawPushConstants), &pushConstants);
		vkCmdBindIndexBuffer(commandBuffer, _testMeshes[_currentMesh]->MeshBuffers.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, _testMeshes[_currentMesh]->Surfaces[0].Count, 1, _testMeshes[_currentMesh]->Surfaces[0].StartIndex, 0, 0);

		vkCmdEndRendering(commandBuffer);
	}

	void App::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		VK_CHECK(vkResetFences(_logicalDevice, 1, &_immFence));
		VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

		VkCommandBuffer cmd = _immCommandBuffer;

		VkCommandBufferBeginInfo beginInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr,
		};

		VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

		function(cmd);

		VK_CHECK(vkEndCommandBuffer(cmd));

		VkCommandBufferSubmitInfo cmdinfo = Vulkan::Init::commandBufferSubmitInfo(cmd);
		VkSubmitInfo2 submit = Vulkan::Init::submitInfo2(cmdinfo, nullptr, nullptr);

		VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immFence));

		VK_CHECK(vkWaitForFences(_logicalDevice, 1, &_immFence, true, UINT64_MAX));
	}


	Vulkan::AllocatedBuffer App::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		VkBufferCreateInfo bufferInfo
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.size = allocSize,
			.usage = usage,
		};

		VmaAllocationCreateInfo vmaallocInfo\
		{
			.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = memoryUsage,
		};

		Vulkan::AllocatedBuffer newBuffer;
		VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.Buffer, &newBuffer.Allocation, &newBuffer.Info));

		return newBuffer;
	}

	void App::DestroyBuffer(const Vulkan::AllocatedBuffer& buffer)
	{
		vmaDestroyBuffer(_allocator, buffer.Buffer, buffer.Allocation);
	}

	Vulkan::GPUMeshBuffers App::UploadMesh(std::span<uint32_t> indices, std::span<Vulkan::Vertex> vertices)
	{
		const size_t vertexBufferSize = vertices.size() * sizeof(Vulkan::Vertex);
		const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

		Vulkan::GPUMeshBuffers newSurface{};

		newSurface.VertexBuffer = CreateBuffer(vertexBufferSize, 
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
				| VK_BUFFER_USAGE_TRANSFER_DST_BIT 
				| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);

		VkBufferDeviceAddressInfo deviceAdressInfo
		{ 
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = newSurface.VertexBuffer.Buffer 
		};

		newSurface.VertexBufferAddress = vkGetBufferDeviceAddress(_logicalDevice, &deviceAdressInfo);

		newSurface.IndexBuffer = CreateBuffer(indexBufferSize, 
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT 
				| VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);

		Vulkan::AllocatedBuffer staging = CreateBuffer(vertexBufferSize + indexBufferSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VMA_MEMORY_USAGE_CPU_ONLY);

		//Stagin Buffer
		void* data = staging.Allocation->GetMappedData();
		memcpy(data, vertices.data(), vertexBufferSize);
		memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

		ImmediateSubmit([&](VkCommandBuffer cmd) {
			VkBufferCopy vertexCopy{ 0 };
			vertexCopy.dstOffset = 0;
			vertexCopy.srcOffset = 0;
			vertexCopy.size = vertexBufferSize;

			vkCmdCopyBuffer(cmd, staging.Buffer, newSurface.VertexBuffer.Buffer, 1, &vertexCopy);

			VkBufferCopy indexCopy{ 0 };
			indexCopy.dstOffset = 0;
			indexCopy.srcOffset = vertexBufferSize;
			indexCopy.size = indexBufferSize;

			vkCmdCopyBuffer(cmd, staging.Buffer, newSurface.IndexBuffer.Buffer, 1, &indexCopy);
		});

		DestroyBuffer(staging);
		return newSurface;
	}

	void App::UploadDefaultMeshData()
	{
		_testMeshes = Vulkan::Loader::loadGltfMeshes(this, "assets/models/basicmesh.glb").value();

		DeletionQueue.Push([=]()
			{
				for (auto& mesh : _testMeshes) {
					DestroyBuffer(mesh->MeshBuffers.IndexBuffer);
					DestroyBuffer(mesh->MeshBuffers.VertexBuffer);
				}
			}
		);
	}
}