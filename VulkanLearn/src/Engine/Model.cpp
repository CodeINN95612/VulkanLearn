#include "Model.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

//This needs to be made so that we can use the Vertex struct as a key in an unordered_map
namespace std {
	template<> struct hash<::Engine::Vertex> {
		size_t operator()(::Engine::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.Pos) ^
				(hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.TexCoord) << 1);
		}
	};
}

namespace Engine
{
	std::shared_ptr<Model> Model::LoadFromFile(const char* path)
	{
		Model* pModel = new Model();

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path);
		if (!loaded)
		{
			throw std::exception("No fue posible cargar el modelo");
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.Pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.TexCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.Color = { 1.0f, 1.0f, 1.0f };

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = (uint32_t)pModel->Vertices.size();
					pModel->Vertices.push_back(vertex);
				}

				pModel->Indices.push_back(uniqueVertices[vertex]);
			}
		}

		return std::shared_ptr<Model>(pModel);
	}
}
