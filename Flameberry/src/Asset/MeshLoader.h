#pragma once

#include <filesystem>

#include "Renderer/StaticMesh.h"

namespace Flameberry {

    class MeshLoader
    {
    public:
        static std::shared_ptr<Asset> LoadMesh(const std::filesystem::path& path);
        static std::shared_ptr<Asset> LoadMeshOBJ(const std::filesystem::path& path);
    };

}