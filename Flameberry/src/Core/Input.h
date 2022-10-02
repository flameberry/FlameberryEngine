#pragma once

#include <glm/glm.hpp>

namespace Flameberry {
    struct Input
    {
        static bool IsKey(uint16_t key, uint16_t action);
        static bool IsMouseButton(uint16_t button, uint16_t action);
        static glm::vec2 GetCursorPosition();
    };
}