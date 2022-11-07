#include "Application.h"

#include "Timer.h"
#include "Core.h"

#include "ImGui/ImGuiLayer.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

namespace Flameberry {
    Application* Application::s_Instance;

    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();
        if (FL_RENDERER_API_CURRENT == FL_RENDERER_API_OPENGL)
            ImGuiLayer::OnAttach();
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

            if (FL_RENDERER_API_CURRENT == FL_RENDERER_API_OPENGL)
            {
                ImGuiLayer::Begin();
                this->OnUIRender();
                ImGuiLayer::End();
            }

            m_Window->OnUpdate();
        }
    }

    Application::~Application()
    {
        if (FL_RENDERER_API_CURRENT == FL_RENDERER_API_OPENGL)
            ImGuiLayer::OnDetach();
        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}
