#pragma once

#include <glm/glm.hpp>
#include "Vulkan/VulkanTexture.h"

#include "Core/UUID.h"

namespace Flameberry {
    struct Material
    {
        std::string Name;
        glm::vec3 Albedo;
        float Roughness;
        bool Metallic;

        bool TextureMapEnabled = false;
        std::shared_ptr<VulkanTexture> TextureMap;

        UUID GetUUID() const { return m_UUID; }

        Material() = default;
        Material(
            const std::string& name,
            const glm::vec3& albedo,
            float roughness,
            bool metallic,
            bool textureMapEnabled,
            const std::shared_ptr<VulkanTexture>& textureMap
        ) : Name(name), Albedo(albedo), Roughness(roughness), Metallic(metallic), TextureMapEnabled(textureMapEnabled), TextureMap(textureMap)
        {}
    private:
        UUID m_UUID;
    };
}