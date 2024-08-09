#pragma once

#include <Renderer/Renderer.h>

#include "Camera.h"

class App
{
public:
	App();
	virtual ~App() = default;

	void Run();
	void InitWindow();
	void Loop();
	void Shutdown();

	void OnUpdate(float dt);
	void OnResize(uint32_t width, uint32_t height);
	void OnScroll(double yoffset);
	void OnMouseMove(double xPos, double yPos);

private:
	std::unique_ptr<vl::core::Renderer> _renderer = nullptr;
	GLFWwindow* _pWindow = nullptr;

	uint32_t _width = 1400;
	uint32_t _height = 800;

	bool _doRender = true;
	double _fps = 0.0f;

	glm::vec4 _clearColor{ 0.007f, 0.007f, 0.007f, 1.f };

	std::vector<glm::vec3> _cubePositions;
	std::vector<glm::vec4> _cubeColors;

	Camera _camera;

private:
	void OnImguiRender();

	void AddCube();
	void GenerateChunk();
};