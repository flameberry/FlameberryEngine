#include "Window.h"
#include <iostream>
#include <glad/glad.h>

namespace Flameberry {
    std::shared_ptr<Window> Window::Create(int width, int height, const char* title)
    {
        return std::make_shared<Window>(width, height, title);
    }

    Window::Window(int width, int height, const char* title)
        : M_Width(width), M_Height(height), M_Title(title)
    {
        if (!glfwInit())
            std::cout << "Failed to Initialize GLFW!" << std::endl;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

        M_Window = glfwCreateWindow(width, height, title, NULL, NULL);

        if (!M_Window)
            std::cout << "Window is null!" << std::endl;

        glfwMakeContextCurrent(M_Window);
        glfwSwapInterval(1);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
        }

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