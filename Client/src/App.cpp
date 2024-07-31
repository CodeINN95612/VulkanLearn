#include "App.h"
#include <imgui.h>

App::App()
{

}

void App::Run()
{
	InitWindow();

	_renderer = vl::core::Renderer::Create(_pWindow, _width, _height);
	_renderer->Init();

	Loop();

	Shutdown();
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
	_pWindow = glfwCreateWindow(_width, _height, "Vulkan Triangle", nullptr, nullptr);
	//glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(_pWindow, this);

	glfwSetFramebufferSizeCallback(_pWindow, [](GLFWwindow* window, int width, int height)
		{
			auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
			app->OnResize(width, height);
		}
	);

	/*glfwSetScrollCallback(_window, [](GLFWwindow* window, double xoffset, double yoffset)
		{
			auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
			app->OnScroll(yoffset);
		}
	);*/

	if (!_pWindow) {
		throw std::exception("Error al crear la ventana");
	}
}

void App::Loop()
{
	double lastTime = glfwGetTime();
	double deltaTime = 0.0;
	double lastFrame = 0.0;
	int frameCount = 0;

	while (!glfwWindowShouldClose(_pWindow)) {
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();

		//OnUpdate(_deltaTime);


		if (_doRender)
		{
			_renderer->OnImGuiRender([this]() 
				{
					OnImguiRender();
				}
			);

			_renderer->SetClearColor(_clearColor);

			_renderer->StartFrame();

			_renderer->DrawTriangle();

			_renderer->SubmitFrame();
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
}

void App::Shutdown()
{
	_renderer->Shutdown();

	glfwDestroyWindow(_pWindow);
	glfwTerminate();
}

void App::OnResize(uint32_t width, uint32_t height)
{
	_width = width;
	_height = height;

	_doRender = width != 0 && height != 0;
	_renderer->OnResize(width, height);
}

void App::OnImguiRender()
{
	if (ImGui::Begin("Utils"))
	{
		ImGui::Text("FPS: %.1f", _fps);

		ImGui::Text("Width: %d", _width);
		ImGui::Text("Height: %d", _height);

		ImGui::ColorEdit3("Clear Color", &_clearColor.x);
	}
	ImGui::End();
}
