#include "App.h"
#include <imgui.h>

App::App() : _camera(_width, _height)
{
}

void App::Run()
{
	InitWindow();

	_renderer = vl::core::Renderer::Create(_pWindow, _width, _height);
	_renderer->Init();
	_cubeBuffer = _renderer->GetCubeBuffer();

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
	_pWindow = glfwCreateWindow(_width, _height, "Vulkan Triangle", nullptr, nullptr);

	_showCursor = false;
	glfwSetInputMode(_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetWindowUserPointer(_pWindow, this);

	glfwSetFramebufferSizeCallback(_pWindow, [](GLFWwindow* window, int width, int height)
		{
			auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
			app->OnResize(width, height);
		}
	);

	glfwSetScrollCallback(_pWindow, [](GLFWwindow* window, double xoffset, double yoffset)
		{
			auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
			app->OnScroll(yoffset);
		}
	);

	glfwSetCursorPosCallback(_pWindow, [](GLFWwindow* window, double xpos, double ypos)
		{
			auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
			app->OnMouseMove(xpos, ypos);
		}
	);

	glfwSetKeyCallback(_pWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
			app->OnKeyPressed(key, scancode, action, mods);
		}
	);

	if (!_pWindow) {
		throw std::exception("Error al crear la ventana");
	}
}

void App::Loop()
{
	VL_PROFILE_BEGIN_SESSION("App Loop", "loop.vlprof.json");

	double lastTime = glfwGetTime();
	double deltaTime = 0.0;
	double lastFrame = 0.0;
	int frameCount = 0;

	GenerateChunk();

	while (!glfwWindowShouldClose(_pWindow)) {
		VL_PROFILE_SCOPE("Frame");
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		{
			VL_PROFILE_SCOPE("glfwPollEvents");
			glfwPollEvents();
		}

		{
			VL_PROFILE_SCOPE("OnUpdate");
			OnUpdate((float)deltaTime);
		}

		if (_doRender)
		{
			VL_PROFILE_SCOPE("Render");

			_renderer->OnImGuiRender([this]() 
				{
					VL_PROFILE_SCOPE("OnImguiRender");
					OnImguiRender();
				}
			);

			_renderer->SetClearColor(_clearColor);

			_renderer->StartFrame(_camera.GetViewProjectionMatrix());
			_renderer->SubmitFrame();
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// Calculate FPS
		{
			VL_PROFILE_SCOPE("CalculateFPS");
			double currentTime = glfwGetTime();
			frameCount++;
			if (currentTime - lastTime >= 0.5) {
				_fps = frameCount / (currentTime - lastTime);
				frameCount = 0;
				lastTime = currentTime;
			}
		}
	}

	VL_PROFILE_END_SESSION();
}

void App::Shutdown()
{
	_renderer->Shutdown();

	glfwDestroyWindow(_pWindow);
	glfwTerminate();
}

static float secondTimer = 0;
void App::OnUpdate(float dt)
{
	VL_PROFILE_FUNCTION();
	_camera.OnUpdate(_pWindow, dt);

	return;
	secondTimer += dt;
	static int x = 0;
	static int y = 0;

	//if (secondTimer > 0.2f)
	/*{
		const int size = 100;
		secondTimer = 0;

		if (y > size)
		{
			return;
		}

		for (int z = 0; z < size; z++)
		{
			glm::mat4 tranform = glm::translate(glm::mat4(1.0f), { x, y, z });
			_cubes.push_back({ tranform, { x / float(size), y / float(size), z / float(size), 1.0f } });
		}

		x++;

		if (x > size)
		{
			x = 0;
			y++;
		}
	}*/
}

void App::OnResize(uint32_t width, uint32_t height)
{
	_width = width;
	_height = height;

	_doRender = width != 0 && height != 0;
	_renderer->OnResize(width, height);
}

void App::OnScroll(double yoffset)
{
	_camera.OnScroll((float)yoffset);
}

void App::OnMouseMove(double xPos, double yPos)
{
	static double lastX = xPos;
	static double lastY = yPos;

	double xoffset = xPos - lastX;
	double yoffset = lastY - yPos;

	_camera.OnMouseMove((float)xoffset, (float)yoffset);

	lastX = xPos;
	lastY = yPos;

}

void App::OnKeyPressed(int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(_pWindow, GLFW_TRUE);
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		_showCursor = !_showCursor;
		glfwSetInputMode(_pWindow, GLFW_CURSOR, _showCursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
}

void App::OnImguiRender()
{
	if (ImGui::Begin("Utils"))
	{
		ImGui::Text("FPS: %.1f", _fps);

		ImGui::Text("Width: %d", _width);
		ImGui::Text("Height: %d", _height);

		ImGui::ColorEdit3("Clear Color", &_clearColor.x);

		if (ImGui::Button("Insert Cube"))
		{
			AddCube();
		}

		ImGui::Separator();
		ImGui::Text("Camera");
		float sensitivity = _camera.GetSensitivity();
		ImGui::SliderFloat("Sensitivity",  &sensitivity, 0.1f, 10.0f);
		_camera.SetSensitivity(sensitivity);
	}
	ImGui::End();
}

void App::AddCube()
{
	//random grid positon between -5 and 5
	float x = static_cast<float>(rand() % 10) - 5.0f;
	float y = static_cast<float>(rand() % 10) - 5.0f;
	float z = static_cast<float>(rand() % 10) - 5.0f;

	float r = static_cast<float>(rand() % 255) / 255.0f;
	float g = static_cast<float>(rand() % 255) / 255.0f;
	float b = static_cast<float>(rand() % 255) / 255.0f;
	float a = static_cast<float>(rand() % 255) / 255.0f;

	glm::mat4 tranform = glm::translate(glm::mat4(1.0f), { x, y, z });
	_cubeBuffer->InsertCube({ tranform, { r, g, b, a } });
}

void App::GenerateChunk()
{
	const int size = 100;
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			for (int k = 0; k < size; k++)
			{
				float x = i;
				float y = j;
				float z = k;
				
				float divisor = size + 1.0f;
				glm::mat4 tranform = glm::translate(glm::mat4(1.0f), { x, y, z });
				_cubeBuffer->InsertCube({ tranform, { x / divisor, y / divisor, z / divisor, 1.0f } });
			}
		}
	}
}
