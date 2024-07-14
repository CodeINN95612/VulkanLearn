#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	Camera();
	virtual ~Camera() {}

	void OnScroll(float yoffset);
	glm::mat4 GetViewMatrix() const;

private:
	glm::vec3 _position = { 2.0f, 2.0f, 2.0f };
	glm::vec3 _front = { 0.01f, 0.01f, 0.01f };
	glm::vec3 _up = {0.0f, 0.0f, 1.0f};

	float _speed = 10.0f;

};