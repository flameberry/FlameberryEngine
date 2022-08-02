#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "Core.h"

namespace Flameberry {
    class Input
    {
    public:
        static bool IsKey(uint16_t key, uint16_t action);
        static bool IsMouseButton(uint16_t button, uint16_t action);
    private:
        static GLFWwindow* GetCachedWindow();
    private:
        static GLFWwindow* m_GLFWwindowCache;
    };
}