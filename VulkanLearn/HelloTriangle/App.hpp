#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace HelloTriangle
{
	class App
	{
	public:
		App();
		virtual ~App() {}

		void Run();

	public:
		static const uint32_t WIDTH = 800;
		static const uint32_t HEIGHT = 600;

	private:
		void InitWindow();
		void InitVulkan();
		void Loop();
		void Clean();

		void CreateInstance();

	private:
		GLFWwindow* _window = nullptr;
		VkInstance _instance = VK_NULL_HANDLE;
	};
}