#include "Input.h"
#include "Core/Application.h"

namespace Flameberry {
    bool Input::IsKeyPressed(KeyCode key)
    {
        GLFWwindow* window = Application::Get().GetWindow().GetGLFWwindow();
        return glfwGetKey(window, (uint16_t)key) == GLFW_PRESS;
    }

    bool Input::IsMouseButtonPressed(uint16_t button)
    {
        GLFWwindow* window = Application::Get().GetWindow().GetGLFWwindow();
        return glfwGetMouseButton(window, button) == GLFW_PRESS;
    }

    glm::vec2 Input::GetCursorPosition()
    {
        GLFWwindow* window = Application::Get().GetWindow().GetGLFWwindow();
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        return glm::vec2((float)x, (float)y);
    }

    void Input::SetCursorPosition(const glm::vec2& pos)
    {
        GLFWwindow* window = Application::Get().GetWindow().GetGLFWwindow();
        glfwSetCursorPos(window, pos.x, pos.y);
    }

    void Input::SetCursorMode(int mode)
    {
        GLFWwindow* window = Application::Get().GetWindow().GetGLFWwindow();
        glfwSetInputMode(window, GLFW_CURSOR, mode);
    }
}
