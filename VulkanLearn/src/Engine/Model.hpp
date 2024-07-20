#pragma once

#include <vector>
#include <tinyobjloader/tiny_obj_loader.h>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

namespace Engine
{
	class Vertex 
	{
	public:
		glm::vec3 Pos;
		glm::vec3 Color;
		glm::vec2 TexCoord;

		inline bool operator==(const Vertex& other) const {
			return Pos == other.Pos && Color == other.Color && TexCoord == other.TexCoord;
		}
	};

	class Model 
	{
	public:
		virtual ~Model() = default;

		static std::shared_ptr<Model> LoadFromFile(const char* path);
	private:
		Model() = default;

	private:
		std::vector<::Engine::Vertex> Vertices;
		std::vector<uint32_t> Indices;
	};
}