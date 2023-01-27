#include "Application.h"

#include "Timer.h"
#include "Core.h"

namespace Flameberry {
    Application* Application::s_Instance;

    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();
        m_Window->SetEventCallBack(FL_BIND_EVENT_FN(Application::OnEvent));
        m_ImGuiLayer.OnCreate();
    }

    void Application::Run()
    {
        float last = 0.0f;
        while (m_Window->IsRunning())
        {
            float now = glfwGetTime();
            float delta = now - last;
            last = now;

            for (auto& layer : m_LayerStack)
                layer->OnUpdate(delta);

            m_ImGuiLayer.Begin();
            for (auto& layer : m_LayerStack)
                layer->OnUIRender();
            m_ImGuiLayer.End();

            m_Window->OnUpdate();
        }
    }

    void Application::OnEvent(Event& e)
    {
        switch (e.GetType()) {
        case EventType::KEY_PRESSED:
            this->OnKeyPressedEvent(*(KeyPressedEvent*)(&e));
            break;
        case EventType::NONE:
            break;
        }

        m_ImGuiLayer.OnEvent(e);

        for (auto& layer : m_LayerStack)
        {
            layer->OnEvent(e);
            if (e.Handled)
                break;
        }
    }

    void Application::OnKeyPressedEvent(KeyPressedEvent& e)
    {
    }

    Application::~Application()
    {
        for (auto& layer : m_LayerStack)
            layer->OnDestroy();
        m_ImGuiLayer.OnDestroy();

        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}
