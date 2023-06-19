#pragma once

#include <glm/glm.hpp>
#include "Vulkan/Texture2D.h"

#include "Core/UUID.h"

namespace Flameberry {
    struct Material
    {
    public:
        // Meta Data
        std::string Name = "Default_Material", FilePath;
        bool IsDerived = false;

        // Actual properties
        glm::vec3 Albedo{ 1.0f };
        float Roughness = 0.2f;
        float Metallic = 0.0f;
        bool TextureMapEnabled = false, NormalMapEnabled = false, RoughnessMapEnabled = false, AmbientOcclusionMapEnabled = false, MetallicMapEnabled = false;
        std::shared_ptr<Texture2D> TextureMap, NormalMap, RoughnessMap, AmbientOcclusionMap, MetallicMap;
    public:
        static std::shared_ptr<Material> LoadFromFile(const char* path);
        UUID GetUUID() const { return m_UUID; }

        Material() = default;
        Material(UUID uuid);
    private:
        UUID m_UUID;
    };

    class MaterialSerializer
    {
    public:
        static void Serialize(const std::shared_ptr<Material>& material, const char* path);
        static std::shared_ptr<Material> Deserialize(const char* path);
    };
}