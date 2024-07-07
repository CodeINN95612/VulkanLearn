#include <spdlog/spdlog.h>

#include "App.hpp"

namespace HelloTriangle
{
	App::App() :
		_window(nullptr)
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

