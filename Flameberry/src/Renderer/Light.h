#pragma once

#include <glm/glm.hpp>

namespace Flameberry {
    struct DirectionalLight
    {
        alignas(16) glm::vec3 Direction;
        alignas(16) glm::vec3 Color;
        alignas(4) float Intensity, LightSize;
    };

    struct PointLight
    {
        alignas(16) glm::vec3 Position;
        alignas(16) glm::vec3 Color;
        alignas(4) float Intensity;
    };
}
