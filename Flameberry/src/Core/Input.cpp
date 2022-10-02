#include "Input.h"
#include "Core/Application.h"

namespace Flameberry {
    bool Input::IsKey(uint16_t key, uint16_t action)
    {
        GLFWwindow* window = Application::Get().GetWindow().GetGLFWwindow();
        return glfwGetKey(window, key) == action;
    }

    bool Input::IsMouseButton(uint16_t button, uint16_t action)
    {
        GLFWwindow* window = Application::Get().GetWindow().GetGLFWwindow();
        return glfwGetMouseButton(window, button) == action;
    }

    glm::vec2 Input::GetCursorPosition()
    {
        GLFWwindow* window = Application::Get().GetWindow().GetGLFWwindow();
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        return glm::vec2((float)x, (float)y);
    }
}