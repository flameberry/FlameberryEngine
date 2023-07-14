#pragma once

#include <glm/glm.hpp>
#include "Texture2D.h"

#include "Core/UUID.h"

namespace Flameberry {
    class Material : public Asset
    {
    public:
        std::string Name = "Default_Material";
        glm::vec3 Albedo{ 1.0f };
        float Roughness = 0.2f, Metallic = 0.0f;
        bool TextureMapEnabled = false, NormalMapEnabled = false, RoughnessMapEnabled = false, AmbientOcclusionMapEnabled = false, MetallicMapEnabled = false;
        std::shared_ptr<Texture2D> TextureMap, NormalMap, RoughnessMap, AmbientOcclusionMap, MetallicMap;

        AssetType GetAssetType() const override { return AssetType::Material; }
        static constexpr AssetType GetStaticAssetType() { return AssetType::Material; }
    };

    class MaterialSerializer
    {
    public:
        static void Serialize(const std::shared_ptr<Material>& material, const char* path);
        static std::shared_ptr<Material> Deserialize(const char* path);
    };
}