#include "Application.h"
#include <glad/glad.h>
#include "Core.h"
#include "../renderer/Renderer2D.h"

namespace Flameberry {
    Application* Application::S_Instance;
    Application::Application()
    {
        S_Instance = this;
        M_Window = Window::Create();

        Renderer2DInitInfo rendererInitInfo{};
        rendererInitInfo.enableFontRendering = false;
        rendererInitInfo.userWindow = M_Window->GetGLFWwindow();
        rendererInitInfo.enableCustomViewport = true;
        rendererInitInfo.customViewportSize = { 1280.0f, 720.0f };

        Renderer2D::Init(rendererInitInfo);

        M_FlameEditor.OnAttach();
    }

    void Application::Run()
    {
        while (M_Window->IsRunning())
        {
            M_FlameEditor.OnRender();

            M_FlameEditor.OnImGuiBegin();
            M_FlameEditor.OnImGuiRender();
            M_FlameEditor.OnImGuiEnd();

            M_Window->OnUpdate();
        }
    }

    Application::~Application()
    {
        M_FlameEditor.OnDetach();
        Renderer2D::CleanUp();
        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}