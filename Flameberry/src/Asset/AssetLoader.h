#pragma once

#include "Renderer/StaticMesh.h"
#include "Renderer/Material.h"

namespace Flameberry {
    class AssetLoader
    {
    public:
        static std::shared_ptr<Asset> LoadAsset(const std::filesystem::path& path, AssetType type);
    };
}