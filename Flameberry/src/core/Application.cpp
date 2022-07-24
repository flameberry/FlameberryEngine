#include "Application.h"
#include <glad/glad.h>
#include "Core.h"
#include "../renderer/Renderer.h"

namespace Flameberry {
    Application::Application()
    {
        M_Window = Window::Create();

        RendererInitInfo rendererInitInfo{};
        rendererInitInfo.enableFontRendering = false;
        rendererInitInfo.userWindow = M_Window->GetGLFWwindow();

        Renderer::Init(rendererInitInfo);
    }

    void Application::Run()
    {
        while (M_Window->IsRunning())
        {
            glClear(GL_COLOR_BUFFER_BIT);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

            Renderer::Begin();
            Renderer::AddQuad({ 0, 0, 0 }, { 100, 100 }, FL_PINK, FL_ELEMENT_TYPE_GENERAL_INDEX);
            Renderer::End();

            M_Window->OnUpdate();
        }

    }

    Application::~Application()
    {
        Renderer::CleanUp();
        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}