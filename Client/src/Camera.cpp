#include "Camera.h"

Camera::Camera(uint32_t width, uint32_t height):
	_width(width),
	_height(height)
{
	UpdateCameraVectors();
	UpdateProjectionMatrix();
	UpdateViewMatrix();
	UpdateViewProjectionMatrix();
}

void Camera::OnUpdate(GLFWwindow* pWindow, float dt)
{
	float speed = _speed * dt;
	if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS)
		_position += speed * _front;
	if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS)
		_position -= speed * _front;
	if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS)
		_position -= glm::normalize(glm::cross(_front, _up)) * speed;
	if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS)
		_position += glm::normalize(glm::cross(_front, _up)) * speed;

	UpdateViewMatrix();
}

void Camera::OnScroll(float yOffset)
{
	_fov -= (float)yOffset;
	if (_fov < 1.0f)
		_fov = 1.0f;
	if (_fov > MAX_FOV)
		_fov = MAX_FOV;
	UpdateProjectionMatrix();
}

void Camera::OnMouseMove(float xOffset, float yOffset)
{
	float sensitivity = _sensitivity * SENSITIVITY * _fov / MAX_FOV;
	_yaw += xOffset * sensitivity;
	_pitch += yOffset * sensitivity;

	if (_pitch > 89.0f)
		_pitch = 89.0f;
	if (_pitch < -89.0f)
		_pitch = -89.0f;

	UpdateCameraVectors();
	UpdateViewMatrix();
}

void Camera::OnResize(uint32_t width, uint32_t height)
{
	_width = width;
	_height = height;

	UpdateProjectionMatrix();
}

const glm::mat4& Camera::GetViewMatrix() const
{
	return _viewMatrix;
}

const glm::mat4& Camera::GetProjectionMatrix() const
{
	return _projectionMatrix;
}

const glm::mat4& Camera::GetViewProjectionMatrix() const
{
	return _viewProjectionMatrix;
}

void Camera::UpdateCameraVectors()
{
	glm::vec3 direction{};
	direction.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	direction.y = sin(glm::radians(_pitch));
	direction.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	_front = glm::normalize(direction);
}

void Camera::UpdateProjectionMatrix()
{
	_projectionMatrix = glm::perspective(glm::radians(_fov), (float)_width / (float)_height, 0.01f, 10000.0f);
	_projectionMatrix[1][1] *= -1;
	UpdateViewProjectionMatrix();
}

void Camera::UpdateViewMatrix()
{
	_viewMatrix = glm::lookAt(_position, _position + _front, _up);
	UpdateViewProjectionMatrix();
}

void Camera::UpdateViewProjectionMatrix()
{
	_viewProjectionMatrix = _projectionMatrix * _viewMatrix;
}
