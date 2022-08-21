#include "Window.h"

#include <iostream>
#include <glad/glad.h>

#include "Core.h"

namespace Flameberry {
    std::shared_ptr<Window> Window::Create(int width, int height, const char* title)
    {
        return std::make_shared<Window>(width, height, title);
    }

    Window::Window(int width, int height, const char* title)
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

        m_Window = glfwCreateWindow(width, height, title, NULL, NULL);

        FL_ASSERT(m_Window, "GLFW Window is Null!");

        glfwMakeContextCurrent(m_Window);
        glfwSwapInterval(1);

        FL_ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to Initialize Glad!");

        int actualWidth, actualHeight;
        glfwGetFramebufferSize(m_Window, &actualWidth, &actualHeight);
        glViewport(0, 0, actualWidth, actualHeight);

        void* pointer;
        glfwSetWindowUserPointer(m_Window, pointer);

        // Setting events
        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xoffset, double yoffset) {
            }
        );
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
        glfwSwapBuffers(m_Window);
        glfwPollEvents();
    }
}