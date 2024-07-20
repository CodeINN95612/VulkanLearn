#include "App.hpp"

#include <set>
#include <fstream>
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
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
		: _camera(WIDTH, HEIGHT)
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
		_framebufferResized = resized;
		_width = width;
		_height = height;

		bool minimized = width == 0 || height == 0;
		_doRender = !minimized;
		_camera.OnResize(width, height);
	}

	void App::OnScroll(double yoffset)
	{
		_camera.OnScroll((float)yoffset);
	}

	void App::InitWindow()
	{
		spdlog::info("Inicializando Ventana");

		if (glfwInit() == GLFW_FALSE)
		{
			throw std::exception("Error al inicializar glfw");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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

		glfwSetCursorPosCallback(_window, [](GLFWwindow* window, double xpos, double ypos)
			{
				auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

				static double lastX = xpos;
				static double lastY = ypos;

				double xoffset = xpos - lastX;
				double yoffset = lastY - ypos;

				app->GetCamera().OnMouseMove((float)xoffset, (float)yoffset);

				lastX = xpos;
				lastY = ypos;
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

		DeletionQueue.Flush();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(_logicalDevice, _frames[i].ImageAvailableSemaphore, nullptr);
			vkDestroySemaphore(_logicalDevice, _frames[i].RenderFinishedSemaphore, nullptr);
			vkDestroyFence(_logicalDevice, _frames[i].Fence, nullptr);
			vkDestroyCommandPool(_logicalDevice, _frames[i].CommandPool, nullptr);

			_frames[i].DeletionQueue.Flush();
		}

		CleanUpSwapChain();

		Vulkan::BoostrapData data = {
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

		if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)
		{
			Engine::InputState::Up = true;
		}
		else
		{
			Engine::InputState::Up = false;
		}

		if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)
		{
			Engine::InputState::Down = true;
		}
		else
		{
			Engine::InputState::Down = false;
		}

		if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS)
		{
			Engine::InputState::Left = true;
		}
		else
		{
			Engine::InputState::Left = false;
		}

		if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS)
		{
			Engine::InputState::Right = true;
		}
		else
		{
			Engine::InputState::Right = false;
		}

		_camera.OnUpdate(_deltaTime);
	}

	void App::OnImGuiRender()
	{
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

		ImGui::Begin("FPS");
		ImGui::Text("FPS: %.1f", _fps);
		ImGui::End();

		//ImGui::ShowDemoWindow();

		ImGui::End();

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
	}

	void App::CreateSwapChain()
	{
		auto data = Vulkan::boostrapSwapchain(_width, _height, _physicalDevice, _logicalDevice, _surface);
		_swapChain = data.swapchain;
		_swapChainImageFormat = data.imageFormat;
		_swapChainExtent = data.extent;
		_swapChainImages = data.images;
		_swapChainImageViews = data.imageViews;

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

		VmaAllocationCreateInfo drawImageAllocInfo
		{
			.usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		};

		VK_CHECK(vmaCreateImage(_allocator, &drawImageInfo, &drawImageAllocInfo, &_drawImage.Image, &_drawImage.Allocation, nullptr));

		VkImageViewCreateInfo drawImageViewInfo = Vulkan::Init::imageViewCreateInfo(
			_drawImage.ImageFormat,
			_drawImage.Image,
			VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(_logicalDevice, &drawImageViewInfo, nullptr, &_drawImage.ImageView));

		DeletionQueue.Push([=]()
			{
				vkDestroyImageView(_logicalDevice, _drawImage.ImageView, nullptr);
				vmaDestroyImage(_allocator, _drawImage.Image, _drawImage.Allocation);
			}
		);
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
		return;

		VK_CHECK(vkDeviceWaitIdle(_logicalDevice));

		CleanUpSwapChain();

		CreateSwapChain();
	}

	void App::CleanUpSwapChain()
	{
		for (auto imageView : _swapChainImageViews) {
			vkDestroyImageView(_logicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(_logicalDevice, _swapChain, nullptr);
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

		DeletionQueue.Push([=]()
			{
				vkDestroyDescriptorSetLayout(_logicalDevice, _descriptorSetLayout, nullptr);
				_descriptorAllocator.DestroyPool(_logicalDevice);
			}
		);
	}

	void App::CreatePipeline()
	{
		//Compute Shader
		VkPipelineLayoutCreateInfo pipelineLayoutInfo
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &_descriptorSetLayout,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr,
		};

		VK_CHECK(vkCreatePipelineLayout(_logicalDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout));

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

		VK_CHECK(vkCreateComputePipelines(_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline));

		vkDestroyShaderModule(_logicalDevice, computeShaderModule, nullptr);

		DeletionQueue.Push([=]()
			{
				vkDestroyPipeline(_logicalDevice, _pipeline, nullptr);
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

	void App::DrawFrame()
	{
		VK_CHECK(vkWaitForFences(_logicalDevice, 1, &Frame().Fence, VK_TRUE, UINT64_MAX));

		Frame().DeletionQueue.Flush();

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(_logicalDevice, _swapChain, UINT64_MAX, Frame().ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::exception("No fue posible obtener la imagen");
		}

		VK_CHECK(vkResetFences(_logicalDevice, 1, &Frame().Fence));

		_drawImageExtent.width = _drawImage.ImageExtent.width;
		_drawImageExtent.height = _drawImage.ImageExtent.height;

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
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
			_framebufferResized = false;
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::exception("No fue posible presentar en la cola de presentacion");
		}

		_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void App::DrawImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView)
	{
		VkRenderingAttachmentInfo colorAttachment = Vulkan::Init::attachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
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

		ClearBackground(commandBuffer);

		Vulkan::Image::transitionImage(commandBuffer, _drawImage.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		Vulkan::Image::transitionImage(commandBuffer, _swapChainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		Vulkan::Image::copyImage(commandBuffer, _drawImage.Image, _swapChainImages[imageIndex], _drawImageExtent, _swapChainExtent);

		Vulkan::Image::transitionImage(commandBuffer, _swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		DrawImgui(commandBuffer, _swapChainImageViews[imageIndex]);

		Vulkan::Image::transitionImage(commandBuffer, _swapChainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		VK_CHECK(vkEndCommandBuffer(commandBuffer));
	}

	void App::ClearBackground(VkCommandBuffer commandBuffer)
	{
		/*VkClearColorValue clearColor = { {0.005f, 0.005f, 0.005f, 1.0f} };
		VkImageSubresourceRange clearRange = Vulkan::Init::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

		vkCmdClearColorImage(commandBuffer, _drawImage.Image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);*/


		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);
		vkCmdDispatch(commandBuffer, _drawImageExtent.width / 16, _drawImageExtent.height / 16, 1);
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
}