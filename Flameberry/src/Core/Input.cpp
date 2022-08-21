#include "Input.h"
#include "Core/Application.h"

namespace Flameberry {
    bool Input::IsKey(uint16_t key, uint16_t action)
    {
        GLFWwindow* window = Application::Get().GetWindow().GetGLFWwindow();
        if (glfwGetKey(window, key) == action)
            return true;
        return false;
    }

    bool Input::IsMouseButton(uint16_t button, uint16_t action)
    {
        GLFWwindow* window = Application::Get().GetWindow().GetGLFWwindow();
        if (glfwGetMouseButton(window, button) == action)
            return true;
        return false;
    }
}