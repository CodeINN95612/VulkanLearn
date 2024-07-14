#include "Camera.hpp"

Camera::Camera()
{
}

void Camera::OnScroll(float yoffset)
{
	_position += -1 * yoffset * _speed * _front;
}

glm::mat4 Camera::GetViewMatrix() const
{
	return glm::lookAt(_position, _front, _up);
}
