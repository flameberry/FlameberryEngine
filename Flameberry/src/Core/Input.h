#pragma once

#include <glm/glm.hpp>

#include "Core/KeyCodes.h"

namespace Flameberry {
    struct Input
    {
        static bool IsKeyPressed(KeyCode key);
        static bool IsMouseButtonPressed(uint16_t button);
        static glm::vec2 GetCursorPosition();
        static void SetCursorPosition(const glm::vec2& pos);
        static void SetCursorMode(uint32_t mode);
    };
}
