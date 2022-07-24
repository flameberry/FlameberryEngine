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
        : M_Width(width), M_Height(height), M_Title(title)
    {
        FL_ASSERT(glfwInit(), "Failed to Initialize GLFW!");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

        M_Window = glfwCreateWindow(width, height, title, NULL, NULL);

        FL_ASSERT(M_Window, "GLFW Window is Null!");

        glfwMakeContextCurrent(M_Window);
        glfwSwapInterval(1);

        FL_ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to Initialize Glad!");

        int actualWidth, actualHeight;
        glfwGetFramebufferSize(M_Window, &actualWidth, &actualHeight);
        glViewport(0, 0, actualWidth, actualHeight);
    }

    Window::~Window()
    {
        glfwDestroyWindow(M_Window);
    }

    bool Window::IsRunning()
    {
        return !glfwWindowShouldClose(M_Window);
    }

    void Window::OnUpdate()
    {
        glfwSwapBuffers(M_Window);
        glfwPollEvents();
    }
}