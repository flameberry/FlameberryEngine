#pragma once

#include <glm/glm.hpp>

namespace Flameberry {
    struct PointLight
    {
        glm::vec3 Position;
        glm::vec4 Color;
        float Intensity;

        PointLight()
            : Position(0.0f), Color(1.0f), Intensity(1.0f)
        {}

        PointLight(const glm::vec3& position, const glm::vec4& color, float intensity)
            : Position(position), Color(color), Intensity(intensity)
        {}
    };
}