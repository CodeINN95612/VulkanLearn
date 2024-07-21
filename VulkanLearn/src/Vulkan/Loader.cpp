#include "Loader.hpp"

#include "../HelloVulkan/App.hpp"
#include "Loader.hpp"
#include <stb/stb_image.h>
#include <iostream>

#include "Init.hpp"
#include "Types.hpp"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

using namespace fastgltf;

namespace Vulkan::Loader
{

    std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(HelloVulkan::App* engine, std::filesystem::path filePath)
    {
        ("Loading GLTF: {}", filePath);

        GltfDataBuffer data;
        data.loadFromFile(filePath);

        constexpr auto gltfOptions = Options::LoadGLBBuffers | Options::LoadExternalBuffers;

        Asset gltf;
        Parser parser{};

        auto load = parser.loadBinaryGLTF(&data, filePath.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        }
        else {
            spdlog::error("Failed to load glTF: {0} \n", to_underlying(load.error()));
            return {};
        }

        std::vector<std::shared_ptr<MeshAsset>> meshes;

        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;

        for (fastgltf::Mesh& mesh : gltf.meshes) {
            MeshAsset newmesh;
            newmesh.Name = mesh.name;

            indices.clear();
            vertices.clear();

            for (auto&& p : mesh.primitives) {
                GeoSurface newSurface;
                newSurface.StartIndex = (uint32_t)indices.size();
                newSurface.Count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

                size_t initial_vtx = vertices.size();

                Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                    [&](std::uint32_t idx) {
                        indices.push_back(idx + uint32_t(initial_vtx));
                    });

                Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                    [&](glm::vec3 v, size_t index) {
                        Vertex newvtx
                        {
                            .Position = v,
                            .Uv_x = 0,
                            .Normal = { 1, 0, 0 },
                            .Uv_y = 0,
                            .Color = glm::vec4{ 1.f },
                        };
                        vertices[initial_vtx + index] = newvtx;
                    });

                auto normals = p.findAttribute("NORMAL");
                if (normals != p.attributes.end()) {

                    iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                        [&](glm::vec3 v, size_t index) {
                            vertices[initial_vtx + index].Normal = v;
                        });
                }

                auto uv = p.findAttribute("TEXCOORD_0");
                if (uv != p.attributes.end()) {

                    iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                        [&](glm::vec2 v, size_t index) {
                            vertices[initial_vtx + index].Uv_x = v.x;
                            vertices[initial_vtx + index].Uv_y = v.y;
                        });
                }

                auto colors = p.findAttribute("COLOR_0");
                if (colors != p.attributes.end()) {

                    iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                        [&](glm::vec4 v, size_t index) {
                            vertices[initial_vtx + index].Color = v;
                        });
                }
                newmesh.Surfaces.push_back(newSurface);
            }

            constexpr bool OverrideColors = true;
            if (OverrideColors) {
                for (Vertex& vtx : vertices) {
                    vtx.Color = glm::vec4(vtx.Normal, 1.f);
                }
            }
            newmesh.MeshBuffers = engine->UploadMesh(indices, vertices);
            meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newmesh)));
        }

        return meshes;
    }

}
