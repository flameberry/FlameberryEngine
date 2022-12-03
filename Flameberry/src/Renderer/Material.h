#pragma once

#include <glm/glm.hpp>

namespace Flameberry {
    struct Material
    {
        glm::vec3 Albedo;
        float Roughness;
        bool IsMetal;

        Material(const glm::vec3& albedo = glm::vec3(1.0f), float roughness = 1.0f, bool isMetal = false) : Albedo(albedo), Roughness(roughness), IsMetal(isMetal) {}
    };
}