#include "Application.h"
#include <glad/glad.h>

#include "Core.h"
#include "Renderer/Renderer2D.h"
#include "Timer.h"
#include "ImGui/ImGuiLayer.h"
#include "ECS/Registry.h"
#include "ECS/Component.h"

namespace Flameberry {
    Application* Application::s_Instance;
    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();
        ImGuiLayer::OnAttach();

        Registry registry;
        Entity entity = registry.CreateEntity();
        registry.AddComponent<TransformComponent>(entity);
        registry.AddComponent<SpriteRendererComponent>(entity);
        Entity entity1 = registry.CreateEntity();
        registry.AddComponent<TransformComponent>(entity1);
        registry.AddComponent<SpriteRendererComponent>(entity1);
        Entity entity2 = registry.CreateEntity();
        registry.AddComponent<TransformComponent>(entity2);
        registry.AddComponent<SpriteRendererComponent>(entity2);
        Entity entity3 = registry.CreateEntity();
        registry.AddComponent<TransformComponent>(entity3);
        registry.AddComponent<SpriteRendererComponent>(entity3);

        // for (auto [transform, sprite] : registry.View<TransformComponent, SpriteRendererComponent>())
        //     FL_LOG(sprite->TextureFilePath);

        utils::sparse_set _set(5, 100);
        _set.insert(10);
        _set.insert(40);
        _set.insert(20);
        _set.insert(50);
        _set.insert(30);

        for (auto& element : _set)
            FL_LOG(element);

        for (utils::sparse_set::iterator it = _set.begin(); it != _set.end(); it++)
            FL_LOG(*it);
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

    Application::~Application()
    {
        ImGuiLayer::OnDetach();
        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}
