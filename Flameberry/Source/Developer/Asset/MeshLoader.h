#pragma once

#include <filesystem>

#include "Renderer/StaticMesh.h"

namespace Flameberry {

    class MeshLoader
    {
    public:
        static Ref<Asset> LoadMesh(const std::filesystem::path& path);
        static Ref<Asset> LoadMeshOBJ(const std::filesystem::path& path);
    };

}