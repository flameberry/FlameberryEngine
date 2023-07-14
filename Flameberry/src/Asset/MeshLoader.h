#pragma once

#include <filesystem>

#include "Renderer/StaticMesh.h"

namespace Flameberry {

    class MeshLoader
    {
    public:
        static std::shared_ptr<Asset> LoadMesh(const std::filesystem::path& path);
    };

}