#include "Window.h"

#include <iostream>

#include "Core.h"

namespace Flameberry {
    std::shared_ptr<Window> Window::Create(int width, int height, const char* title)
    {
        return std::make_shared<Window>(width, height, title);
    }

    Window::Window(int width, int height, const char* title)
        : m_Width(width), m_Height(height), m_Title(title)
    {
#ifdef FL_USE_VULKAN_API
        InitForVulkan();
#elif defined(FL_USE_OPENGL_API)
        InitForOpenGL();
#endif
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_Window);
    }

    bool Window::IsRunning()
    {
        return !glfwWindowShouldClose(m_Window);
    }

    void Window::OnUpdate()
    {
#ifdef FL_USE_OPENGL_API
        glfwSwapBuffers(m_Window);
#endif

        glfwPollEvents();
    }

#ifdef FL_USE_OPENGL_API
#include <glad/glad.h>
    void Window::InitForOpenGL()
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
    }
#elif defined(FL_USE_VULKAN_API)
    void Window::InitForVulkan()
    {
        FL_ASSERT(glfwInit(), "Failed to initialize GLFW!");
        FL_INFO("Initialized GLFW!");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_Window = glfwCreateWindow(m_Width, m_Height, m_Title, NULL, NULL);

        FL_ASSERT(m_Window, "GLFW window is null!");
        FL_INFO("Created GLFW window of title '{0}' and dimensions ({1}, {2})", m_Title, m_Width, m_Height);
    }
#endif
}