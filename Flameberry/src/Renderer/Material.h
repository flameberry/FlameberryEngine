#pragma once

#include <glm/glm.hpp>
#include "Vulkan/VulkanTexture.h"

namespace Flameberry {
    struct Material
    {
        glm::vec3 Albedo;
        float Roughness;
        bool Metallic;
        bool TextureMapEnabled = false;
        std::shared_ptr<VulkanTexture> TextureMap;
    };
}