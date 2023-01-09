#include "Application.h"

#include "Timer.h"
#include "Core.h"

#include "ImGui/ImGuiLayer.h"

namespace Flameberry {
    Application* Application::s_Instance;

    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();
    }

    void Application::Run()
    {
        float last = 0.0f;
        while (m_Window->IsRunning())
        {
            float now = glfwGetTime();
            float delta = now - last;
            last = now;

            this->OnUpdate(delta);
            m_Window->OnUpdate();
        }
    }

    Application::~Application()
    {
        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}
