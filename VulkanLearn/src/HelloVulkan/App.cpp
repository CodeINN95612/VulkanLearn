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

#include "../Engine/InputState.hpp"
#include "../Vulkan/Utils.hpp"
#include "../Vulkan/Init.hpp"
#include "../Vulkan/Image.hpp"
#include "../Common/Utils.hpp"


namespace std {
	template<> struct hash<HelloVulkan::Vertex> {
		size_t operator()(HelloVulkan::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

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
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
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
				OnRender();
			}
			else
			{
				//throttle the speed to avoid the endless spinning
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

            // Calculate FPS
            double currentTime = glfwGetTime();
            frameCount++;
            if (currentTime - lastTime >= 0.5) {
                double fps = frameCount / (currentTime - lastTime);
                std::string windowTitle = "Vulkan Triangle | FPS: " + std::to_string(fps);
                glfwSetWindowTitle(_window, windowTitle.c_str());
				_fps = fps;
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
	}

	void App::RecreateSwapChain()
	{
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

	void App::LoadModel(const char* path)
	{
		/*tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path);
		if (!loaded)
		{
			throw std::exception("No fue posible cargar el modelo");
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.color = { 1.0f, 1.0f, 1.0f };

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = (uint32_t)_vertices.size();
					_vertices.push_back(vertex);
				}

				_indices.push_back(uniqueVertices[vertex]);
			}
		}*/
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

		VkSubmitInfo2 submitInfo2 = Vulkan::Init::submitInfo2(commandBufferInfo, waitSemaphoreSubmitInfo,  signalSemaphoreSubmitInfo);

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

		Vulkan::Image::transitionImage(commandBuffer, _swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		VK_CHECK(vkEndCommandBuffer(commandBuffer));
	}

	void App::ClearBackground(VkCommandBuffer commandBuffer)
	{
		VkClearColorValue clearColor = { {0.005f, 0.005f, 0.005f, 1.0f} };
		VkImageSubresourceRange clearRange = Vulkan::Init::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

		vkCmdClearColorImage(commandBuffer, _drawImage.Image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
	}
}