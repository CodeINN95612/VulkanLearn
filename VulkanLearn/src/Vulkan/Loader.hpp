#pragma once

#include "Types.hpp"

#define _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
#include <filesystem>

namespace HelloVulkan
{
    class App;
}

namespace Vulkan::Loader
{
    struct GeoSurface 
    {
        uint32_t StartIndex;
        uint32_t Count;
    };

    struct MeshAsset {
        std::string Name;

        std::vector<GeoSurface> Surfaces;
        GPUMeshBuffers MeshBuffers;
    };

    std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(HelloVulkan::App* engine, std::filesystem::path filePath);

}