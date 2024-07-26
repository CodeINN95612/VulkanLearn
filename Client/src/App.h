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

private:
	std::unique_ptr<vl::core::Renderer> _renderer = nullptr;
	GLFWwindow* _pWindow = nullptr;

	uint32_t _width = 800;
	uint32_t _height = 600;

	bool _doRender = true;
	double _fps = 0.0f;
};