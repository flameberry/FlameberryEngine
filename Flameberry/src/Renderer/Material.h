#pragma once

#include <glm/glm.hpp>

namespace Flameberry {
    struct Material
    {
        alignas(16) glm::vec3 Albedo;
        alignas(4) float Roughness;
        alignas(4) float Metallic;
    };
}