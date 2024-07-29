#pragma once

#include <Renderer/Renderer.h>

class App
{
public:
	App();
	virtual ~App() = default;

	void Run();
	void InitWindow();
	void Loop();
	void Shutdown();

	void GenerateRendering();

	void OnResize(uint32_t width, uint32_t height);

private:
	std::unique_ptr<vl::core::Renderer> _renderer = nullptr;
	GLFWwindow* _pWindow = nullptr;

	uint32_t _width = 800;
	uint32_t _height = 600;

	bool _doRender = true;
	double _fps = 0.0f;

	std::shared_ptr<vl::core::Shader> _vertexShader = nullptr;
	std::shared_ptr<vl::core::Shader> _fragmentShader = nullptr;

private:
	void OnImguiRender();
};