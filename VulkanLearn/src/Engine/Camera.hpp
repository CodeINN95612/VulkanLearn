#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
	class Camera
	{
	public:
		Camera(uint32_t width, uint32_t height);
		virtual ~Camera() {}

		void OnUpdate(float dt);

		void OnScroll(float yOffset);
		void OnMouseMove(float xOffset, float yOffset);
		void OnResize(uint32_t width, uint32_t height);

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix() const;

	private:
		inline static const float MAX_FOV = 45.0f;
		inline static const float SENSITIVITY = 0.1f;

	private:
		uint32_t _width;
		uint32_t _height;

		glm::vec3 _position = { 0.0f, 3.0f, 0.0f };
		glm::vec3 _front = { 0.0f, -1.0f, 1.0f };
		glm::vec3 _up = {0.0f, 0.0f, 1.0f};
	
		float _fov = 45.0f;
		float _yaw = -90.0f;
		float _pitch = 0.0f;

		float _speed = 10.0f;
		float _sensitivity = 1.0f;

	private:
		void UpdateCameraVectors();
	};
}
