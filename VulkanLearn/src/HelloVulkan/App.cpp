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
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		inline bool IsComplete() const {
			return graphicsFamily.has_value()
				&& presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails 
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	static const std::vector<const char*> requiredLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	static  const std::vector<const char*> requiredDeviceExtensions = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	/*const std::vector<Vertex> _vertices = 
	{
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
		{{ 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{ 0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = 
	{
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};*/

	static void checkRequiredExtensions(const std::vector<const char*>& requiredExtensions);
	static void checkRequiredLayers(const std::vector<const char*>& requiredLayers);
	static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	static bool areExtensionsSupported(VkPhysicalDevice device);
	static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
	static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice);
	bool hasStencilComponent(VkFormat format);

	static std::vector<char> readFile(const std::string& filename);
	

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
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Triangle", nullptr, nullptr);
		glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetWindowUserPointer(_window, this);

		glfwSetFramebufferSizeCallback(_window, [](GLFWwindow* window, int width, int height)
			{
				spdlog::info("Cambio de tamaño de ventana: {0}, {1}", width, height);
				auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
				app->SetResized(true);
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
		while (!glfwWindowShouldClose(_window)) {

			float currentFrame = static_cast<float>(glfwGetTime());
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

			if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			{
				if (glfwGetInputMode(_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
				{
					glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
				else
				{
					glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				}
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

			_camera.OnUpdate(deltaTime);

			DrawFrame();

			glfwPollEvents();
		}

		VkResult result = vkDeviceWaitIdle(_logicalDevice);
		if (result != VK_SUCCESS)
		{
			throw std::exception("Error al esperar para el dispositivo al finalizar");
		}
	}

	void App::Clean()
	{
		spdlog::info("Limpiando");

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(_logicalDevice, _renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(_logicalDevice, _imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(_logicalDevice, _inFlightFences[i], nullptr);
		}

		vkDestroyBuffer(_logicalDevice, _indexBuffer, nullptr);
		vkFreeMemory(_logicalDevice, _indexBufferMemory, nullptr);

		vkDestroyBuffer(_logicalDevice, _vertexBuffer, nullptr);
		vkFreeMemory(_logicalDevice, _vertexBufferMemory, nullptr);

		vkDestroyCommandPool(_logicalDevice, _commandPool, nullptr);

		vkDestroyPipeline(_logicalDevice, _graphicsPipeline, nullptr);

		vkDestroyPipelineLayout(_logicalDevice, _pipelineLayout, nullptr);

		vkDestroyRenderPass(_logicalDevice, _renderPass, nullptr);

		CleanUpSwapChain();

		vkDestroySampler(_logicalDevice, _textureSampler, nullptr);
		vkDestroyImageView(_logicalDevice, _textureImageView, nullptr);
		vkDestroyImage(_logicalDevice, _textureImage, nullptr);
		vkFreeMemory(_logicalDevice, _textureImageMemory, nullptr);

		vkDestroyDescriptorPool(_logicalDevice, _descriptorPool, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(_logicalDevice, _uniformBuffers[i], nullptr);
			vkFreeMemory(_logicalDevice, _uniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorSetLayout(_logicalDevice, _descriptorSetLayout, nullptr);

		vkDestroyDevice(_logicalDevice, nullptr);

		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		DestroyMessenger();

		vkDestroyInstance(_instance, nullptr);

		glfwDestroyWindow(_window);
		glfwTerminate();

		spdlog::info("Limpiada exitosa");
	}

	void App::InitVulkan()
	{
		CreateInstance();
		CreateMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateCommandPool();
		CreateTextureImage(TEXTURE_PATH.c_str());
		CreateTextureImageView();
		CreateTextureSampler();
		CreateDescriptorSetLayout();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateSwapChain();
		CreateDepthResources();
		CreateImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFramebuffers();
		LoadModel(MODEL_PATH.c_str());
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateCommandBuffers();
		CreateSyncObjects();
	}

	void App::CreateInstance()
	{
		spdlog::info("Inicializando Instancia");

		VkApplicationInfo appInfo =
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Hello Vulkan",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "No Engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_0,
		};

		std::vector<const char*> requiredExtensions = 
		{
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		
		for (uint32_t i = 0; i < glfwExtensionCount; i++)
		{
			requiredExtensions.push_back(glfwExtensions[i]);
		}

		checkRequiredExtensions(requiredExtensions);

		checkRequiredLayers(requiredLayers);

		VkInstanceCreateInfo createInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = (uint32_t)requiredLayers.size(),
			.ppEnabledLayerNames = requiredLayers.data(),
			.enabledExtensionCount = (uint32_t)requiredExtensions.size(),
			.ppEnabledExtensionNames = requiredExtensions.data(),
		};

		VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);
		if (result != VK_SUCCESS)
		{
			throw std::exception("Error al crear la instancia");
		}

		spdlog::info("Inicializacion de Instancia exitosa");

	}

	void App::CreateMessenger()
	{
		spdlog::info("Inicializando Sistema de mensajeria");

		VkDebugUtilsMessengerCreateInfoEXT createInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
				,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
				| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
			.pfnUserCallback = messageCallback,
			.pUserData = nullptr,
		};

		auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
		
		if (!vkCreateDebugUtilsMessengerEXT)
		{
			throw std::exception("Error al crear la funcion vkCreateDebugUtilsMessengerEXT");
		}
		
		VkResult result = vkCreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_messenger);
		if (result != VK_SUCCESS)
		{
			throw std::exception("Error al crear el sistema de mensajeria");
		}

		spdlog::info("Inicializacion de Sitema de mensajeria exitosa");
	}

	void App::DestroyMessenger()
	{
		auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXT != nullptr) {
			vkDestroyDebugUtilsMessengerEXT(_instance, _messenger, nullptr);
		}
	}

	void App::PickPhysicalDevice()
	{
		spdlog::info("Escogiendo Dispositivo fisico");

		uint32_t deviceCount = 0;
		VkResult result = vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
		if (result != VK_SUCCESS || deviceCount == 0)
		{
			throw std::exception("vkEnumeratePhysicalDevices fallo su primera llamada");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		result = vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());
		if (result != VK_SUCCESS)
		{
			throw std::exception("vkEnumeratePhysicalDevices fallo su segunda llamada");
		}

		VkPhysicalDevice suitableDevice = VK_NULL_HANDLE;
		for (uint32_t i = 0; i < deviceCount; i++)
		{
			if (isDeviceSuitable(devices[i], _surface))
			{
				suitableDevice = devices[i];
				break;
			}
		}

		if (!suitableDevice) 
		{
			throw std::exception("No se encontro un dispositivo fisico valido");
		}

		_physicalDevice = suitableDevice;

		spdlog::info("Escogida de Dispositivo fisico exitosa");
	}

	void App::CreateLogicalDevice()
	{
		spdlog::info("Inicializando Dispositivo logico");

		QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, _surface);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = 
		{ 
			indices.graphicsFamily.value(), 
			indices.presentFamily.value() 
		};

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = 
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = queueFamily,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority,
			};
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = (uint32_t)queueCreateInfos.size(),
			.pQueueCreateInfos = queueCreateInfos.data(),
			.enabledLayerCount = (uint32_t)requiredLayers.size(),
			.ppEnabledLayerNames = requiredLayers.data(),
			.enabledExtensionCount = (uint32_t)requiredDeviceExtensions.size(),
			.ppEnabledExtensionNames = requiredDeviceExtensions.data(),
			.pEnabledFeatures = &deviceFeatures,
		};

		VkResult result = vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_logicalDevice);
		if (result != VK_SUCCESS)
		{
			throw std::exception("Error al crear el dispositivo logico");
		}

		vkGetDeviceQueue(_logicalDevice, indices.graphicsFamily.value(), 0, &_graphicsQueue);
		if (_graphicsQueue == VK_NULL_HANDLE)
		{
			throw std::exception("Error al obtener las colas de graficos");
		}

		vkGetDeviceQueue(_logicalDevice, indices.presentFamily.value(), 0, &_presentQueue);
		if (_presentQueue == VK_NULL_HANDLE)
		{
			throw std::exception("Error al obtener las colas de presentacion");
		}

		spdlog::info("Inicializacion del Dispositivo logico exitosa");
	}

	void App::CreateSurface()
	{
		spdlog::info("Inicializando Superficie");

		VkWin32SurfaceCreateInfoKHR createInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = GetModuleHandle(nullptr),
			.hwnd = glfwGetWin32Window(_window),
		};

		VkResult result = vkCreateWin32SurfaceKHR(_instance, &createInfo, nullptr, &_surface);
		if (result != VK_SUCCESS)
		{
			throw std::exception("Error al crear la superficie");
		}

		spdlog::info("Inicializacion de Superficie exitosa");
	}

	void App::CreateSwapChain()
	{
		spdlog::info("Inicializando SwapChain");

		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice, _surface);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, _window);

		uint32_t imageCount = 3;
		if (swapChainSupport.capabilities.maxImageCount != 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = _surface,
			.minImageCount = imageCount,
			.imageFormat = surfaceFormat.format,
			.imageColorSpace = surfaceFormat.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		};

		QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, _surface);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult result = vkCreateSwapchainKHR(_logicalDevice, &createInfo, nullptr, &_swapChain);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible crear el SwapChain");
		}

		result = vkGetSwapchainImagesKHR(_logicalDevice, _swapChain, &imageCount, nullptr);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible obtener imagenes en la primera llamada");
		}

		_swapChainImages.resize(imageCount);
		result = vkGetSwapchainImagesKHR(_logicalDevice, _swapChain, &imageCount, _swapChainImages.data());
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible obtener imagenes en la segunda llamada");
		}

		_swapChainImageFormat = surfaceFormat.format;
		_swapChainExtent = extent;

		spdlog::info("Inicializacion del SwapChain exitosa");
	}

	void App::CreateImageViews()
	{
		spdlog::info("Inicializando Vistas a imagen");

		_swapChainImageViews.resize(_swapChainImages.size());

		for (size_t i = 0; i < _swapChainImages.size(); i++) 
		{
			_swapChainImageViews[i] = CreateImageView(_swapChainImages[i], _swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}

		spdlog::info("Inicializacion de Vistas a imagen exitosa");
	}

	void App::CreateRenderPass()
	{
		spdlog::info("Inicializando RenderPass");

		VkAttachmentDescription colorAttachment
		{
			.format = _swapChainImageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		VkAttachmentDescription depthAttachment
		{
			.format = FindDepthFormat(),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference colorAttachmentRef
		{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference depthAttachmentRef
		{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef,
			.pDepthStencilAttachment = &depthAttachmentRef,
		};

		VkSubpassDependency dependency
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		};

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassInfo
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = (uint32_t)attachments.size(),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};

		VkResult result = vkCreateRenderPass(_logicalDevice, &renderPassInfo, nullptr, &_renderPass);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible crear el Render Pass");
		}

		spdlog::info("Inicializacion de RenderPass exitosa");
	}

	void App::CreateGraphicsPipeline()
	{
		spdlog::info("Inicializando Graphics pipeline");

		auto vertShaderCode = readFile("assets/shaders/shader.vert.spv");
		auto fragShaderCode = readFile("assets/shaders/shader.frag.spv");

		VkShaderModule vertexShaderModule = CreateShaderModule(vertShaderCode);
		VkShaderModule fragmentShaderModule = CreateShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertexShaderStageInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule,
			.pName = "main"
		};

		VkPipelineShaderStageCreateInfo fragmentShaderStageInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragmentShaderModule,
			.pName = "main"
		};

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

		std::vector<VkDynamicState> dynamicStates = 
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = (uint32_t)dynamicStates.size(),
			.pDynamicStates = dynamicStates.data(),
		};

		auto bindingDescription = Vertex::GetBindingDescription();
		auto attributeDescriptions = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size(),
			.pVertexAttributeDescriptions = attributeDescriptions.data(),
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE,
		};

		VkViewport viewport
		{
			.x = 0.0f,
			.y = 0.0f,
			.width = (float)_swapChainExtent.width,
			.height = (float)_swapChainExtent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		VkRect2D scissor
		{
			.offset = { 0, 0 },
			.extent = _swapChainExtent,
		};

		VkPipelineViewportStateCreateInfo viewportState
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		};

		VkPipelineRasterizationStateCreateInfo rasterizer
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f,
		};

		VkPipelineMultisampleStateCreateInfo multisampling
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE,
		};

		VkPipelineColorBlendAttachmentState colorBlendAttachment
		{
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT 
				| VK_COLOR_COMPONENT_G_BIT 
				| VK_COLOR_COMPONENT_B_BIT 
				| VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineColorBlendStateCreateInfo colorBlending
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
			.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f}
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &_descriptorSetLayout,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr,
		};

		VkResult result = vkCreatePipelineLayout(_logicalDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible crear el PipelineLayout");
		}

		VkPipelineDepthStencilStateCreateInfo depthStencil
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f,
		};

		VkGraphicsPipelineCreateInfo pipelineInfo
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 2,
			.pStages = shaderStages,
			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicState,
			.layout = _pipelineLayout,
			.renderPass = _renderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1,
		};

		result = vkCreateGraphicsPipelines(_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible crear la Graphics pipeline");
		}

		vkDestroyShaderModule(_logicalDevice, vertexShaderModule, nullptr);
		vkDestroyShaderModule(_logicalDevice, fragmentShaderModule, nullptr);

		spdlog::info("Inicializacion de Graphics pipeline exitosa");
	}

	void App::CreateFramebuffers()
	{
		spdlog::info("Inicializando Framebuffers");

		_framebuffers.resize(_swapChainImageViews.size());

		for (size_t i = 0; i < _swapChainImageViews.size(); i++) {

			std::array<VkImageView, 2> attachments = {
				_swapChainImageViews[i],
				_depthImageView
			};

			VkFramebufferCreateInfo framebufferInfo
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = _renderPass,
				.attachmentCount = (uint32_t)attachments.size(),
				.pAttachments = attachments.data(),
				.width = _swapChainExtent.width,
				.height = _swapChainExtent.height,
				.layers = 1,
			};

			VkResult result = vkCreateFramebuffer(_logicalDevice, &framebufferInfo, nullptr, &_framebuffers[i]);

			if (result != VK_SUCCESS) {
				throw std::exception("No fue posible crear el framebuffer");
			}
		}

		spdlog::info("Inicializacion de Framebuffers exitosa");
	}

	void App::CreateCommandPool()
	{
		spdlog::info("Inicializando Command Pool");

		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(_physicalDevice, _surface);

		VkCommandPoolCreateInfo poolInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
		};

		VkResult result = vkCreateCommandPool(_logicalDevice, &poolInfo, nullptr, &_commandPool);
		if (result != VK_SUCCESS) {
			throw std::exception("No fue posible crear el Command Pool");
		}

		spdlog::info("Inicializacion de Command Pool exitosa");
	}

	void App::CreateCommandBuffers()
	{
		spdlog::info("Inicializando Command Buffer");

		_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = _commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = (uint32_t)_commandBuffers.size(),
		};

		VkResult result = vkAllocateCommandBuffers(_logicalDevice, &allocInfo, _commandBuffers.data());
		if (result != VK_SUCCESS) {
			throw std::exception("No fue posible crear el Command Buffer");
		}

		spdlog::info("Inicializacion de Command Buffer exitosa");
	}

	void App::CreateSyncObjects()
	{
		spdlog::info("Inicializando Objetos de syncronizacion");

		VkSemaphoreCreateInfo semaphoreInfo
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		VkFenceCreateInfo fenceInfo
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkResult result = vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]);
			if (result != VK_SUCCESS) {
				throw std::exception("No fue posible crear el Semaforo de imagen disponible");
			}

			result = vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]);
			if (result != VK_SUCCESS) {
				throw std::exception("No fue posible crear el Semaforo de renderizacion finalizada");
			}

			result = vkCreateFence(_logicalDevice, &fenceInfo, nullptr, &_inFlightFences[i]);
			if (result != VK_SUCCESS) {
				throw std::exception("No fue posible crear el Fence");
			}
		}
		
		spdlog::info("Inicializacion de Objetos de sincronizacion exitosa");
	}

	void App::RecreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(_window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(_window, &width, &height);
			glfwWaitEvents();
		}

		VkResult result = vkDeviceWaitIdle(_logicalDevice);
		if (result != VK_SUCCESS)
		{
			throw std::exception("Error al esperar para el dispositivo al recrear el swapchain");
		}

		CleanUpSwapChain();

		CreateSwapChain();
		CreateImageViews();
		CreateDepthResources();
		CreateFramebuffers();
	}

	void App::CleanUpSwapChain()
	{
		vkDestroyImageView(_logicalDevice, _depthImageView, nullptr);
		vkDestroyImage(_logicalDevice, _depthImage, nullptr);
		vkFreeMemory(_logicalDevice, _depthImageMemory, nullptr);

		for (auto framebuffer : _framebuffers) {
			vkDestroyFramebuffer(_logicalDevice, framebuffer, nullptr);
		}

		for (auto imageView : _swapChainImageViews) {
			vkDestroyImageView(_logicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(_logicalDevice, _swapChain, nullptr);
	}

	void App::CreateVertexBuffer()
	{
		spdlog::info("Inicializando Buffer de _vertices");

		size_t size = sizeof(_vertices[0]) * _vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, 
			stagingBufferMemory);

		void* data;
		VkResult result = vkMapMemory(_logicalDevice, stagingBufferMemory, 0, size, 0, &data);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible mapear la memoria del Buffer de vertices");
		}

		memcpy(data, _vertices.data(), size);

		vkUnmapMemory(_logicalDevice, stagingBufferMemory);

		CreateBuffer(
			size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			_vertexBuffer, 
			_vertexBufferMemory);

		CopyBuffer(stagingBuffer, _vertexBuffer, size);

		vkDestroyBuffer(_logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(_logicalDevice, stagingBufferMemory, nullptr);

		spdlog::info("Inicializacion de Buffer de vertices exitosa");
	}

	void App::CreateIndexBuffer()
	{
		spdlog::info("Inicializando Buffer de Indices");

		size_t size = sizeof(_indices[0]) * _indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, 
			stagingBufferMemory);

		void* data;
		VkResult result = vkMapMemory(_logicalDevice, stagingBufferMemory, 0, size, 0, &data);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible mapear la memoria del Buffer de Indices");
		}

		memcpy(data, _indices.data(), size);

		vkUnmapMemory(_logicalDevice, stagingBufferMemory);

		CreateBuffer(
			size, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			_indexBuffer, 
			_indexBufferMemory);

		CopyBuffer(stagingBuffer, _indexBuffer, size);

		vkDestroyBuffer(_logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(_logicalDevice, stagingBufferMemory, nullptr);

		spdlog::info("Inicializacion de Buffer de Indices exitosa");
	}

	void App::CreateDescriptorSetLayout()
	{
		spdlog::info("Inicializando Descriptor Set Layout");

		VkDescriptorSetLayoutBinding uboLayoutBinding
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr,
		};

		VkDescriptorSetLayoutBinding samplerLayoutBinding
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		};

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = (uint32_t)bindings.size(),
			.pBindings = bindings.data(),
		};

		VkResult result = vkCreateDescriptorSetLayout(_logicalDevice, &layoutInfo, nullptr, &_descriptorSetLayout);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible crear el Descriptor Set Layout");
		}

		spdlog::info("Inicializacion de Descriptor Set Layout exitosa");
	}

	void App::CreateUniformBuffers()
	{
		size_t size = sizeof(UniformBufferObject); 

		_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CreateBuffer(
				size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				_uniformBuffers[i], 
				_uniformBuffersMemory[i]);

			VkResult result = vkMapMemory(_logicalDevice, _uniformBuffersMemory[i], 0, size, 0, &_uniformBuffersMapped[i]);
			if (result != VK_SUCCESS) 
			{
				throw std::exception("No fue posible mapear la memoria del Uniform Buffer");
			}
		}
	}

	void App::CreateDescriptorPool()
	{
		spdlog::info("Inicializando Descriptor Pool");

		VkDescriptorPoolSize uboPoolSize
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
		};

		VkDescriptorPoolSize samplerPoolSize
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
		};

		std::array<VkDescriptorPoolSize, 2> poolSizes = { uboPoolSize, samplerPoolSize };

		VkDescriptorPoolCreateInfo poolInfo
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = (uint32_t) MAX_FRAMES_IN_FLIGHT,
			.poolSizeCount = (uint32_t)poolSizes.size(),
			.pPoolSizes = poolSizes.data(),
		};

		VkResult result = vkCreateDescriptorPool(_logicalDevice, &poolInfo, nullptr, &_descriptorPool);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible crear el Descriptor Pool");
		}

		spdlog::info("Inicializacion de Descriptor Pool exitosa");
	}

	void App::CreateDescriptorSets()
	{
		spdlog::info("Inicializando Descriptor Sets");

		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = _descriptorPool,
			.descriptorSetCount = (uint32_t)MAX_FRAMES_IN_FLIGHT,
			.pSetLayouts = layouts.data(),
		};

		_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		VkResult result = vkAllocateDescriptorSets(_logicalDevice, &allocInfo, _descriptorSets.data());
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible asignar los Descriptor Sets");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo bufferInfo
			{
				.buffer = _uniformBuffers[i],
				.offset = 0,
				.range = sizeof(UniformBufferObject),
			};

			VkDescriptorImageInfo imageInfo
			{
				.sampler = _textureSampler,
				.imageView = _textureImageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			VkWriteDescriptorSet bufferDescriptorWrite
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = _descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &bufferInfo,
			};

			VkWriteDescriptorSet imageDescriptorWrite
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = _descriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfo,
			};

			std::array<VkWriteDescriptorSet, 2> descriptorWrites = { bufferDescriptorWrite, imageDescriptorWrite };

			vkUpdateDescriptorSets(_logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}

		spdlog::info("Inicializacion de Descriptor Sets exitosa");

	}

	void App::CreateTextureImage(const char* path)
	{
		spdlog::info("Inicializando Imagen de Textura de escultura");

		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::exception("No fue posible cargar la textura");
		}
		
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			imageSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, 
			stagingBufferMemory);

		void* data;
		VkResult result = vkMapMemory(_logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible mapear la memoria de la imagen");
		}

		memcpy(data, pixels, static_cast<size_t>(imageSize));

		vkUnmapMemory(_logicalDevice, stagingBufferMemory);

		stbi_image_free(pixels);

		CreateImage({.width = (uint32_t)texWidth,
				.height = (uint32_t)texHeight,
				.format = VK_FORMAT_R8G8B8A8_SRGB,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				.image = _textureImage,
				.imageMemory = _textureImageMemory});

		TransitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, _textureImage, (uint32_t)texWidth, (uint32_t)texHeight);

		TransitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(_logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(_logicalDevice, stagingBufferMemory, nullptr);

		spdlog::info("Inicializacion de Imagen de Textura de escultura exitosa");
	}

	void App::CreateTextureImageView()
	{
		spdlog::info("Inicializando Vista de Imagen de Textura");

		_textureImageView = CreateImageView(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

		spdlog::info("Inicializacion de Vista de Imagen de Textura exitosa");
	}

	void App::CreateTextureSampler()
	{
		spdlog::info("Inicializando Muestreador de Textura");

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(_physicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo
		{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.minLod = 0.0f,
			.maxLod = 0.0f,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE,
		};

		VkResult result = vkCreateSampler(_logicalDevice, &samplerInfo, nullptr, &_textureSampler);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible crear el Muestreador de Textura");
		}

		spdlog::info("Inicializacion de Muestreador de Textura exitosa");
	}

	void App::LoadModel(const char* path)
	{
		tinyobj::attrib_t attrib;
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
		}
	}

	void App::CreateDepthResources()
	{
		spdlog::info("Inicializando Recursos de Profundidad");

		VkFormat depthFormat = FindDepthFormat();
		CreateImage({ .width = _swapChainExtent.width,
					.height = _swapChainExtent.height,
					.format = depthFormat,
					.tiling = VK_IMAGE_TILING_OPTIMAL,
					.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
					.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					.image = _depthImage,
					.imageMemory = _depthImageMemory });

		_depthImageView = CreateImageView(_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		TransitionImageLayout(_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		spdlog::info("Inicializacion de Recursos de Profundidad exitosa");
	}

	void App::DrawFrame()
	{
		VkResult result = vkWaitForFences(_logicalDevice, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible esperar por el fence");
		}

		uint32_t imageIndex;
		result = vkAcquireNextImageKHR(_logicalDevice, _swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::exception("No fue posible obtener la imagen");
		}

		result = vkResetFences(_logicalDevice, 1, &_inFlightFences[_currentFrame]);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible resetear las fences");
		}

		vkResetCommandBuffer(_commandBuffers[_currentFrame], 0);

		RecordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);

		UpdateUniformBuffer(_currentFrame);

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &_imageAvailableSemaphores[_currentFrame],
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &_commandBuffers[_currentFrame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &_renderFinishedSemaphores[_currentFrame],
		};

		result = vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible subir comandos en la cola de graficos");
		}

		VkSwapchainKHR swapChains[] = { _swapChain };
		VkPresentInfoKHR presentInfo
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &_renderFinishedSemaphores[_currentFrame],
			.swapchainCount = 1,
			.pSwapchains = swapChains,
			.pImageIndices = &imageIndex,
			.pResults = nullptr,
		};

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

	VkShaderModule App::CreateShaderModule(const std::vector<char>& code)
	{
		spdlog::info("Inicializando Modulo shader");

		VkShaderModuleCreateInfo createInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = code.size(),
			.pCode = reinterpret_cast<const uint32_t*>(code.data()),
		};

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VkResult result = vkCreateShaderModule(_logicalDevice, &createInfo, nullptr, &shaderModule);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible crear los modulos de Shader");
		}

		spdlog::info("Inicializado Modulo shader correctamente");

		return shaderModule;
	}

	void App::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
			.pInheritanceInfo = nullptr,
		};

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible empezar el grabado de comandos");
		}

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.005f, 0.005f, 0.005f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = _renderPass,
			.framebuffer = _framebuffers[imageIndex],
			.renderArea
			{
				.offset = {0, 0},
				.extent = _swapChainExtent,
			},
			.clearValueCount = (uint32_t)clearValues.size(),
			.pClearValues = clearValues.data(),
		};

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

		VkViewport viewport
		{
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(_swapChainExtent.width),
			.height = static_cast<float>(_swapChainExtent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor
		{
			.offset = { 0, 0 },
			.extent = _swapChainExtent,
		};
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = { _vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, _indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(
			commandBuffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			_pipelineLayout, 
			0, 
			1, 
			&_descriptorSets[_currentFrame], 
			0, 
			nullptr);

		vkCmdDrawIndexed(commandBuffer, (uint32_t)_indices.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		result = vkEndCommandBuffer(commandBuffer);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible finalizar el grabado de comandos");
		}
	}

	void App::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		spdlog::info("Inicializando Buffer de tamaño: {0}", size);

		VkBufferCreateInfo bufferInfo
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		VkResult result = vkCreateBuffer(_logicalDevice, &bufferInfo, nullptr, &buffer);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible crear el Buffer");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(_logicalDevice, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, _physicalDevice)
		};

		result = vkAllocateMemory(_logicalDevice, &allocInfo, nullptr, &bufferMemory);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible asignar memoria al Buffer");
		}

		result = vkBindBufferMemory(_logicalDevice, buffer, bufferMemory, 0);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible asignar memoria al Buffer con la memoria de _vertices");
		}
		spdlog::info("Inicializacion de Buffer exitosa");
	}

	void App::CreateImage(CreateImageParams params)
	{
		VkImageCreateInfo imageInfo
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = params.format,
			.extent
			{
				.width = params.width,
				.height = params.height,
				.depth = 1,
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = params.tiling,
			.usage = params.usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VkResult result = vkCreateImage(_logicalDevice, &imageInfo, nullptr, &params.image);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible crear la imagen de textura");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(_logicalDevice, params.image, &memRequirements);

		VkMemoryAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, params.properties, _physicalDevice),
		};

		result = vkAllocateMemory(_logicalDevice, &allocInfo, nullptr, &params.imageMemory);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible asignar memoria a la imagen de textura");
		}

		result = vkBindImageMemory(_logicalDevice, params.image, params.imageMemory, 0);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible asignar memoria a la imagen de textura en binding");
		}
	}

	void App::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = StartTemporaryCommandBuffer();

		VkBufferCopy copyRegion
		{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = size,
		};
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		EndTemporaryCommandBuffer(commandBuffer);
	}

	void App::UpdateUniformBuffer(size_t currentImage)
	{
		static double startTime = glfwGetTime();
		double currentTime = glfwGetTime();
		double deltaTime = currentTime - startTime;

		_camera.Resize(_swapChainExtent.width, _swapChainExtent.height);

		UniformBufferObject ubo
		{
			.model = glm::rotate(glm::mat4(1.0f), (float)deltaTime * glm::radians(00.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			.view = _camera.GetViewMatrix(),
			.proj = _camera.GetProjectionMatrix(),
		};
		ubo.proj[1][1] *= -1;

		memcpy(_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}

	void App::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = StartTemporaryCommandBuffer();

		VkImageMemoryBarrier barrier
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = 0,
			.dstAccessMask = 0,
			.oldLayout = oldLayout,
			.newLayout = newLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else {
			throw std::exception("Transicion de imagen no soportada");
		}

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (hasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, 
			&barrier
		);

		EndTemporaryCommandBuffer(commandBuffer);
	}

	void App::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = StartTemporaryCommandBuffer();
		
		VkBufferImageCopy region
		{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.imageOffset = { 0, 0, 0 },
			.imageExtent = { width, height, 1 },
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		EndTemporaryCommandBuffer(commandBuffer);
	}

	VkImageView App::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo viewInfo
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.subresourceRange
			{
				.aspectMask = aspectFlags,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		VkImageView imageView = VK_NULL_HANDLE;
		VkResult result = vkCreateImageView(_logicalDevice, &viewInfo, nullptr, &imageView);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible crear la vista de la imagen de textura");
		}

		return imageView;
	}

	VkFormat App::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::exception("No se encontro un formato soportado");
	}

	VkFormat App::FindDepthFormat()
	{
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkCommandBuffer App::StartTemporaryCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = _commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		VkCommandBuffer commandBuffer;
		VkResult result = vkAllocateCommandBuffers(_logicalDevice, &allocInfo, &commandBuffer);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue generar el command buffer para copiar memoria");
		}

		VkCommandBufferBeginInfo beginInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};

		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible empezar el grabado de comandos de copia de memoria");
		}
		
		return commandBuffer;
	}

	void App::EndTemporaryCommandBuffer(VkCommandBuffer commandBuffer)
	{

		VkResult result = vkEndCommandBuffer(commandBuffer);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible finalizar el grabado de comandos de copia de memoria");
		}

		VkSubmitInfo submitInfo
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
		};

		result = vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible subir comandos de copia de memoria a la cola de graficos");
		}

		result = vkQueueWaitIdle(_graphicsQueue);
		if (result != VK_SUCCESS)
		{
			throw std::exception("No fue posible esperar a que la cola de graficos termine de copiar memoria");
		}

		vkFreeCommandBuffers(_logicalDevice, _commandPool, 1, &commandBuffer);
	}

	void checkRequiredExtensions(const std::vector<const  char*>& requiredExtension)
	{
		uint32_t extensionCount = 0;
		VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		if (result != VK_SUCCESS || extensionCount == 0)
		{
			throw std::exception("vkEnumerateInstanceExtensionProperties fallo su primera llamada");
		}

		std::vector<VkExtensionProperties> extensions(extensionCount);
		result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		if (result != VK_SUCCESS)
		{
			throw std::exception("vkEnumerateInstanceExtensionProperties fallo su segunda llamada");
		}

		spdlog::info("EXTENSIONES DISPONIBLES");
		for (uint32_t i = 0; i < extensionCount; i++)
		{
			spdlog::info("\t- {0}", extensions[i].extensionName);
		}

		spdlog::info("EXTENSIONES REQUERIDAS");
		for (uint32_t i = 0; i < requiredExtension.size(); i++)
		{
			spdlog::info("\t- {0}", requiredExtension[i]);

			bool hasExtensions = false;
			for (uint32_t j = 0; j < extensionCount; j++)
			{
				VkExtensionProperties& current = extensions[j];
				if (strcmp(current.extensionName, requiredExtension[i]) == 0)
				{
					hasExtensions = true;
					break;
				}
			}

			if (!hasExtensions)
			{
				throw std::exception("No se encontraron todas las extensiones requeridas");
			}
		}
	}

	void checkRequiredLayers(const std::vector<const char*>& requiredLayers)
	{
		uint32_t layerCount = 0;
		VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		if (result != VK_SUCCESS || layerCount == 0)
		{
			throw std::exception("vkEnumerateInstanceLayerProperties fallo su primera llamad");
		}

		std::vector<VkLayerProperties> availableLayers(layerCount);
		result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		if (result != VK_SUCCESS)
		{
			throw std::exception("vkEnumerateInstanceLayerProperties fallo su segunda llamad");
		}

		spdlog::info("CAPAS DISPONIBLES");
		for (uint32_t i = 0; i < layerCount; i++)
		{
			spdlog::info("\t- {0}", availableLayers[i].layerName);
		}

		spdlog::info("CAPAS REQUERIDAS");

		for (uint32_t i = 0; i < requiredLayers.size(); i++)
		{
			spdlog::info("\t- {0}", requiredLayers[i]);

			bool hasLayers = false;
			for (uint32_t j = 0; j < layerCount; j++)
			{
				VkLayerProperties& current = availableLayers[j];
				if (strcmp(current.layerName, requiredLayers[i]) == 0)
				{
					hasLayers = true;
					break;
				}
			}

			if (!hasLayers)
			{
				throw std::exception("No se encontraron todas las capas requeridas");
			}
		}
	}

	bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		spdlog::info("Dispositivo fisico encontrado {0} {1}", deviceProperties.deviceID, deviceProperties.deviceName);

		if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			return false;
		}

		auto queueFamilies = findQueueFamilies(device, surface);
		if (!queueFamilies.IsComplete())
		{
			return false;
		}

		bool extensionSupported = areExtensionsSupported(device);
		if (!extensionSupported)
		{
			return false;
		}

		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
		bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		if (!swapChainAdequate)
		{
			return false;
		}

		if (!deviceFeatures.samplerAnisotropy)
		{
			return false;
		}

		spdlog::info("Dispositivo fisico escogido {0} {1}", deviceProperties.deviceID, deviceProperties.deviceName);
		return true;
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport)
			{
				indices.presentFamily = i;
			}

			if (indices.IsComplete()) 
			{
				break;
			}
		}

		return indices;
	}

	bool areExtensionsSupported(VkPhysicalDevice device)
	{
		uint32_t extensionCount = 0;
		VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		if (result != VK_SUCCESS || extensionCount == 0)
		{
			return false;
		}

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		if (result != VK_SUCCESS)
		{
			return false;
		}

		/*spdlog::info("EXTENSIONES DE DISPOSITIVO DISPONIBLES");
		for (uint32_t i = 0; i < extensionCount; i++)
		{
			spdlog::info("\t- {0}", availableExtensions[i].extensionName);
		}*/

		std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
		for (uint32_t i = 0; i < extensionCount; i++) 
		{
			requiredExtensions.erase(availableExtensions[i].extensionName);
			if (requiredExtensions.empty())
			{
				break;
			}
		}

		return requiredExtensions.empty();
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
		{
			return capabilities.currentExtent;
		}
		else 
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::exception("No se encontro un tipo de memoria adecuado");
	}

	bool hasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			spdlog::error("No se pudo abrir el archivo {0}", filename);
			throw std::exception("No se pudo abrir el archivo");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}


}