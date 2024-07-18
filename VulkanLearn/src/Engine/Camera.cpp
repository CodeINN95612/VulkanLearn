#include "Camera.hpp"
#include "InputState.hpp"
#include <spdlog/spdlog.h>

namespace Engine
{
	Camera::Camera(uint32_t width, uint32_t height)
		: _width(width), _height(height)
	{
		UpdateCameraVectors();
	}

	void Camera::OnUpdate(float dt)
	{
		float speed = _speed * dt;

		if (Engine::InputState::Up)
		{
			_position += speed * _front;
		}
		if (Engine::InputState::Down)
		{
			_position -= speed * _front;
		}
		if (Engine::InputState::Left)
		{
			_position -= glm::normalize(glm::cross(_front, _up)) * speed;
		}
		if (Engine::InputState::Right)
		{
			_position += glm::normalize(glm::cross(_front, _up)) * speed;
		}
	}

	void Camera::OnScroll(float yOffset)
	{
		_fov -= (float)yOffset;
		if (_fov < 1.0f)
			_fov = 1.0f;
		if (_fov > MAX_FOV)
			_fov = MAX_FOV;
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
    }

	void Camera::OnResize(uint32_t width, uint32_t height)
	{
		_width = width;
		_height = height;
	}

	glm::mat4 Camera::GetViewMatrix() const
	{
		return glm::lookAt(_position, _position + _front, _up);
	}

	glm::mat4 Camera::GetProjectionMatrix() const
	{
		return glm::perspective(glm::radians(_fov), (float)_width / _height, 0.01f, 100.0f);
	}

	void Camera::UpdateCameraVectors()
	{
		glm::vec3 front;
		front.x = -glm::cos(glm::radians(_yaw)) * glm::cos(glm::radians(_pitch));
		//front.x = 0.0f;
		front.z = glm::sin(glm::radians(_pitch));
		//front.z = 0.0f;
		front.y = glm::sin(glm::radians(_yaw)) * glm::cos(glm::radians(_pitch));
		_front = glm::normalize(front);
	}
}