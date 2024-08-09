#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera
{
public:
	Camera(uint32_t width, uint32_t height);
	virtual ~Camera() {}

	void OnUpdate(GLFWwindow* pWindow, float dt);

	void OnScroll(float yOffset);
	void OnMouseMove(float xOffset, float yOffset);
	void OnResize(uint32_t width, uint32_t height);

	const glm::mat4& GetViewMatrix() const;
	const glm::mat4& GetProjectionMatrix() const;
	const glm::mat4& GetViewProjectionMatrix() const;

	const glm::vec3& GetPosition() const { return _position; }
	const glm::vec3& GetFront() const { return _front; }
	const glm::vec3& GetUp() const { return _up; }

	inline float GetSensitivity() { return _sensitivity; }
	inline void SetSensitivity(float sensitivity) { _sensitivity = sensitivity; }

private:
	inline static const float MAX_FOV = 90.0f;
	inline static const float SENSITIVITY = 0.1f;

private:
	uint32_t _width;
	uint32_t _height;

	glm::vec3 _position = { 0.0f, 0.0f, 5.0f };
	glm::vec3 _front = { 0.0f, -1.0f, 1.0f };
	glm::vec3 _up = { 0.0f, 1.0f, 0.0f };

	glm::mat4 _viewMatrix = glm::mat4(1.0f);
	glm::mat4 _projectionMatrix = glm::mat4(1.0f);
	glm::mat4 _viewProjectionMatrix = glm::mat4(1.0f);

	float _fov = 45.0f;
	float _yaw = -90.0f;
	float _pitch = 0.0f;

	float _speed = 10.0f;
	float _sensitivity = 1.0f;

private:
	void UpdateCameraVectors();

	void UpdateProjectionMatrix();
	void UpdateViewMatrix();
	void UpdateViewProjectionMatrix();
};