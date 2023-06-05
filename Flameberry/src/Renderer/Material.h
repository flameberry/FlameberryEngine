#pragma once

#include <glm/glm.hpp>
#include "Vulkan/VulkanTexture.h"

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
        bool Metallic = false;
        bool TextureMapEnabled = false, NormalMapEnabled = false;
        std::shared_ptr<VulkanTexture> TextureMap, NormalMap;
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