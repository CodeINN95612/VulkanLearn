#include "App.hpp"
#include <set>

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
			return VK_FALSE;
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

namespace HelloTriangle
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

	static  const std::vector<const char*> requiredDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	static void checkRequiredExtensions(const std::vector<const char*>& requiredExtensions);
	static void checkRequiredLayers(const std::vector<const char*>& requiredLayers);
	static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	static bool areExtensionsSupported(VkPhysicalDevice device);
	static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

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

	void App::InitWindow()
	{
		spdlog::info("Inicializando Ventana");

		if (glfwInit() == GLFW_FALSE)
		{
			throw std::exception("Error al inicializar glfw");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Triangle", nullptr, nullptr);

		if (!_window) {
			throw std::exception("Error al crear la ventana");
		}

		spdlog::info("Inicializacion de Ventana exitosa");
	}

	void App::Loop()
	{
		while (!glfwWindowShouldClose(_window)) {
			glfwPollEvents();
		}
	}

	void App::Clean()
	{
		spdlog::info("Limpiando");

		vkDestroySwapchainKHR(_logicalDevice, _swapChain, nullptr);

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
		CreateSwapChain();
	}

	void App::CreateInstance()
	{
		spdlog::info("Inicializando Instancia");

		VkApplicationInfo appInfo =
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Hello Triangle",
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

		spdlog::info("Inicializacion del SwapChain exitosa");
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
}


