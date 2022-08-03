#include "Application.h"
#include <glad/glad.h>
#include "Core.h"
#include "../renderer/Renderer2D.h"

namespace Flameberry {
    Application* Application::s_Instance;
    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();

        Renderer2DInitInfo rendererInitInfo{};
        rendererInitInfo.enableFontRendering = false;
        rendererInitInfo.userWindow = m_Window->GetGLFWwindow();
        rendererInitInfo.enableCustomViewport = true;
        rendererInitInfo.customViewportSize = { 1280.0f, 720.0f };

        Renderer2D::Init(rendererInitInfo);

        m_FlameEditor.OnAttach();
    }

    void Application::Run()
    {
        while (m_Window->IsRunning())
        {
            m_FlameEditor.OnRender();

            m_FlameEditor.OnImGuiBegin();
            m_FlameEditor.OnImGuiRender();
            m_FlameEditor.OnImGuiEnd();

            m_Window->OnUpdate();
        }
    }

    Application::~Application()
    {
        m_FlameEditor.OnDetach();
        Renderer2D::CleanUp();
        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}