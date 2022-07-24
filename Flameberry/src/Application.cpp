#include "Application.h"
#include <glad/glad.h>

namespace Flameberry {
    Application::Application()
    {
        M_Window = Window::Create();
    }

    void Application::Run()
    {
        while (M_Window->IsRunning())
        {
            glClear(GL_COLOR_BUFFER_BIT);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            M_Window->OnUpdate();
        }

    }

    Application::~Application()
    {
        glfwTerminate();
    }
}