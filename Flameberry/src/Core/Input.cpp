#include "Input.h"
#include "Renderer/Renderer2D.h"

namespace Flameberry {
    GLFWwindow* Input::m_GLFWwindowCache = NULL;

    bool Input::IsKey(uint16_t key, uint16_t action)
    {
        if (glfwGetKey(GetCachedWindow(), key) == action)
            return true;
        return false;
    }

    bool Input::IsMouseButton(uint16_t button, uint16_t action)
    {
        if (glfwGetMouseButton(GetCachedWindow(), button) == action)
            return true;
        return false;
    }

    GLFWwindow* Input::GetCachedWindow()
    {
        if (!m_GLFWwindowCache)
            m_GLFWwindowCache = Renderer2D::GetUserGLFWwindow();
        return m_GLFWwindowCache;
    }
}