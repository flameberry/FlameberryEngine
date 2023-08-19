#pragma once

#include <glm/glm.hpp>

namespace Flameberry {
    struct Input
    {
        static bool IsKeyPressed(uint16_t key);
        static bool IsMouseButtonPressed(uint16_t button);
        static glm::vec2 GetCursorPosition();
        static void SetCursorMode(uint32_t mode);
    };
}