#include "Application.h"

#include "Timer.h"
#include "Core.h"

#include "ImGui/ImGuiLayer.h"
#include "ECS/pool.h"

namespace Flameberry {
    Application* Application::s_Instance;

    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();
        if (FL_RENDERER_API_CURRENT == FL_RENDERER_API_OPENGL)
            ImGuiLayer::OnAttach();

        // Testing the new `component_pool` class
#if 0
        component_pool _pool;
        _pool.allocate(sizeof(SpriteRendererComponent), MAX_ENTITIES);
        for (uint32_t i = 0; i < 10; i++)
            _pool.add(entity_handle{ i });
        FL_LOG(_pool.size());
#endif
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
