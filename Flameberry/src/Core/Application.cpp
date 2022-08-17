#include "Application.h"
#include <glad/glad.h>
#include "Core.h"
#include "Renderer/Renderer2D.h"
#include "Timer.h"

namespace Flameberry {
    Application* Application::s_Instance;
    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();

        // m_Renderer3D = Renderer3D::Create();
        // m_Renderer3D->Init(m_Window->GetGLFWwindow());

        // PerspectiveCameraInfo cameraInfo{};
        // cameraInfo.aspectRatio = 1280.0f / 720.0f;
        // cameraInfo.FOV = 45.0f;
        // cameraInfo.cameraPostion = glm::vec3(0, 0, 2);
        // cameraInfo.cameraDirection = glm::vec3(0, 0, -1);

        // m_Camera = PerspectiveCamera(cameraInfo);

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
            // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

            // int width, height;
            // glfwGetWindowSize(m_Window->GetGLFWwindow(), &width, &height);

            // m_Camera.SetAspectRatio((float)width / (float)height);
            // m_Camera.OnUpdate();

            // m_Renderer3D->Begin(m_Camera);
            // m_Renderer3D->OnDraw();
            // m_Renderer3D->End();

            m_FlameEditor.OnRender();
            m_FlameEditor.OnImGuiRender();

            m_Window->OnUpdate();
        }
    }

    Application::~Application()
    {
        m_FlameEditor.OnDetach();
        Renderer2D::CleanUp();
        // m_Renderer3D->CleanUp();
        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}