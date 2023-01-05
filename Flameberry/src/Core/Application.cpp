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

            ImGuiLayer::Begin();
            this->OnUIRender();
            ImGuiLayer::End();

            m_Window->OnUpdate();
        }
    }

    // void Application::OnKeyPress(uint32_t key)
    // {
    //     m_Window->SetKeyCallBack([](GLFWwindow* window, int key, int scancode, int action, int mods) {});
    // }

    Application::~Application()
    {
        ImGuiLayer::OnDetach();
        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}
