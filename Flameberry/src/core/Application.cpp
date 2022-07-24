#include "Application.h"
#include <glad/glad.h>
#include "Core.h"
#include "../renderer/Renderer2D.h"

namespace Flameberry {
    Application::Application()
    {
        M_Window = Window::Create();

        Renderer2DInitInfo rendererInitInfo{};
        rendererInitInfo.enableFontRendering = false;
        rendererInitInfo.userWindow = M_Window->GetGLFWwindow();

        Renderer2D::Init(rendererInitInfo);
    }

    void Application::Run()
    {
        while (M_Window->IsRunning())
        {
            glClear(GL_COLOR_BUFFER_BIT);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

            Renderer2D::Begin();
            Renderer2D::AddQuad({ 0, 0, 0 }, { 100, 100 }, FL_PINK);
            Renderer2D::AddQuad({ 100, 0, 0 }, { 100, 100 }, FL_PINK, "/Users/flameberry/Developer/FlameUI/Sandbox/resources/textures/Checkerboard.png");
            Renderer2D::AddQuad({ 0, 100, 0 }, { 100, 100 }, FL_BLUE);
            Renderer2D::End();

            M_Window->OnUpdate();
        }
    }

    Application::~Application()
    {
        Renderer2D::CleanUp();
        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}