#pragma once

#include "KeyCodes.h"
#include <glm/glm.hpp>

namespace Flameberry {
	struct Input
	{
		static bool IsKeyPressed(KeyCode key);
		static bool IsMouseButtonPressed(uint16_t button);
		static glm::vec2 GetCursorPosition();
		static void SetCursorPosition(const glm::vec2& pos);
		static void SetCursorMode(int mode);
	};
} // namespace Flameberry
