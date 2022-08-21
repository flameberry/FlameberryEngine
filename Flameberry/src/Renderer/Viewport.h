#pragma once
#include <glm/glm.hpp>

namespace Flameberry {
    struct Viewport
    {
        glm::vec2 size;
        float aspectRatio, windowContentScale;

        Viewport(const glm::vec2& size, float aspectRatio, float windowContentScale = 1.0f)
            : size(size), aspectRatio(aspectRatio), windowContentScale(windowContentScale)
        {}
    };
}