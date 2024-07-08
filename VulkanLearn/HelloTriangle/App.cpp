#include "App.hpp"

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

namespace HelloTriangle
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;

		inline bool IsComplete() const {
			return graphicsFamily.has_value();
		}
	};

	static void checkRequiredExtensions(const std::vector<const char*>& requiredExtensions);
	static void checkRequiredLayers(const std::vector<const char*>& requiredLayers);
	static bool isDeviceSuitable(VkPhysicalDevice device);
	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

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
		PickPhysicalDevice();
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

		const std::vector<const char*> requiredLayers = 
		{
			"VK_LAYER_KHRONOS_validation"
		};

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
			if (isDeviceSuitable(devices[i]))
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

	bool isDeviceSuitable(VkPhysicalDevice device)
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

		auto queueFamilies = findQueueFamilies(device);
		if (!queueFamilies.IsComplete())
		{
			return false;
		}

		spdlog::info("Dispositivo fisico escogido {0} {1}", deviceProperties.deviceID, deviceProperties.deviceName);
		return true;
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
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

			if (indices.IsComplete()) 
			{
				break;
			}
		}

		return indices;
	}
}


