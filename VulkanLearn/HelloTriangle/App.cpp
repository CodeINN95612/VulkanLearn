#include <spdlog/spdlog.h>

#include "App.hpp"

static void checkRequiredExtensions(const std::vector<const char*>& requiredExtension, uint32_t glfwExtensionsCount, const char** glfwExtensions);

namespace HelloTriangle
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

		vkDestroyInstance(_instance, nullptr);

		glfwDestroyWindow(_window);
		glfwTerminate();

		spdlog::info("Limpiada exitosa");

	}

	void App::InitVulkan()
	{
		CreateInstance();
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

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> requiredExtensions = {};

		checkRequiredExtensions(requiredExtensions, glfwExtensionCount, glfwExtensions);

		VkInstanceCreateInfo createInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = 0,
			.enabledExtensionCount = glfwExtensionCount,
			.ppEnabledExtensionNames = glfwExtensions,
		};

		VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);
		if (result != VK_SUCCESS)
		{
			throw std::exception("Error al crear la instancia");
		}

		spdlog::info("Inicializacion de Instancia exitosa");

	}
}

void checkRequiredExtensions(const std::vector<const  char*>& requiredExtension, uint32_t glfwExtensionsCount, const char** glfwExtensions)
{
	uint32_t extensionCount = 0;
	VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	if (result != VK_SUCCESS || extensionCount == 0)
	{
		throw std::exception("vkEnumerateInstanceExtensionProperties failed first call");
	}

	std::vector<VkExtensionProperties> extensions(extensionCount);
	result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	if (result != VK_SUCCESS)
	{
		throw std::exception("vkEnumerateInstanceExtensionProperties failed second call");
	}

	spdlog::info("AVAILABLE EXTENSION");
	for (uint32_t i = 0; i < extensionCount; i++)
	{
		spdlog::info("\t- {0}", extensions[i].extensionName);
	}

	spdlog::info("REQUIRED GLFW EXTENSIONS");
	for (uint32_t i = 0; i < glfwExtensionsCount; i++)
	{
		spdlog::info("\t- {0}", glfwExtensions[i]);

		bool hasGlfwExtensions = false;
		for (uint32_t j = 0; j < extensionCount; j++)
		{
			VkExtensionProperties& current = extensions[j];
			if (strcmp(current.extensionName, glfwExtensions[i]) == 0)
			{
				hasGlfwExtensions = true;
				break;
			}
		}

		if (!hasGlfwExtensions)
		{
			throw std::exception("Vulkan does not have required glfw extensions");
		}
	}

	spdlog::info("REQUIRED EXTENSIONS");
	
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
			throw std::exception("Vulkan does not have required extensions");
		}
	}
}
