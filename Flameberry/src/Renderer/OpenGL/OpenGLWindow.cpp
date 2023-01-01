#include "OpenGLWindow.h"

#include "Core/Core.h"
#include <glad/glad.h>

namespace Flameberry {
    std::shared_ptr<Window> Window::Create(int width, int height, const char* title)
    {
        return std::make_shared<OpenGLWindow>(width, height, title);
    }

    OpenGLWindow::OpenGLWindow(int width, int height, const char* title)
        : m_Width(width), m_Height(height), m_Title(title)
    {
        FL_ASSERT(glfwInit(), "Failed to Initialize GLFW!");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

        m_Window = glfwCreateWindow(m_Width, m_Height, m_Title, NULL, NULL);

        FL_ASSERT(m_Window, "GLFW Window is Null!");

        glfwMakeContextCurrent(m_Window);
        glfwSwapInterval(1);

        FL_ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to Initialize Glad!");

        int actualWidth, actualHeight;
        glfwGetFramebufferSize(m_Window, &actualWidth, &actualHeight);
        glViewport(0, 0, actualWidth, actualHeight);

        // glfwSetKeyCallback(m_Window, m_KeyCallBackFunction);
    }

    OpenGLWindow::~OpenGLWindow()
    {
        glfwDestroyWindow(m_Window);
    }

    void OpenGLWindow::OnUpdate()
    {
        glfwSwapBuffers(m_Window);
        glfwPollEvents();
    }
}