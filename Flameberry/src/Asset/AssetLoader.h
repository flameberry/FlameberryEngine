#pragma once

#include "Renderer/Vulkan/StaticMesh.h"
#include "Renderer/Material.h"

namespace Flameberry {
    class AssetLoader
    {
    public:
        static std::shared_ptr<Asset> LoadAsset(const std::filesystem::path& path);
        static std::shared_ptr<Texture2D> LoadTexture2D(const std::filesystem::path& path);
        static std::shared_ptr<StaticMesh> LoadStaticMesh(const std::filesystem::path& path);
        static std::shared_ptr<Material> LoadMaterial(const std::filesystem::path& path);
    };
}