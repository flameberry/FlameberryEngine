#include "Application.h"

#include "Timer.h"
#include "Core.h"

#include "ImGui/ImGuiLayer.h"
#include "Renderer/Renderer2D.h"

#include "ECS/Registry.h"
#include "ECS/SceneView.h"
#include "ECS/Component.h"

namespace Flameberry {
    Application* Application::s_Instance;

    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();
        ImGuiLayer::OnAttach();

        FL_TRACE("This is a trace message!");

        // Example usage of ECS
        // Registry registry;
        // entity_handle entity = registry.CreateEntity();
        // TransformComponent* comp = registry.AddComponent<TransformComponent>(entity);
        // comp->translation = glm::vec3(0.1f, 0, 0);
        // comp->rotation = glm::vec3(45, 45, 90);
        // comp->scale = glm::vec3(2, 1, 1);
        // registry.AddComponent<SpriteRendererComponent>(entity)->TextureFilePath = "Hello";

        // entity_handle entity1 = registry.CreateEntity();
        // TransformComponent* comp1 = registry.AddComponent<TransformComponent>(entity1);
        // comp1->translation = glm::vec3(0.2f, 0.3f, 0);
        // comp1->rotation = glm::vec3(25, 25, 60);
        // comp1->scale = glm::vec3(2, 2, 3);
        // registry.AddComponent<SpriteRendererComponent>(entity1)->TextureFilePath = "Hi";

        // entity_handle entity2 = registry.CreateEntity();
        // TransformComponent* comp2 = registry.AddComponent<TransformComponent>(entity2);
        // comp2->translation = glm::vec3(0.34f, 0.56f, 0);
        // comp2->rotation = glm::vec3(65, 65, 20);
        // comp2->scale = glm::vec3(3, 2, 1);
        // registry.AddComponent<SpriteRendererComponent>(entity2)->TextureFilePath = "Comeon";

        // entity_handle entity3 = registry.CreateEntity();
        // TransformComponent* comp3 = registry.AddComponent<TransformComponent>(entity3);
        // comp3->translation = glm::vec3(1, 2, 0);
        // comp3->rotation = glm::vec3(100, 100, 600);
        // comp3->scale = glm::vec3(8, 9, 10);
        // registry.AddComponent<SpriteRendererComponent>(entity3)->TextureFilePath = "wow";

        // for (auto [transform, sprite] : registry.View<TransformComponent, SpriteRendererComponent>())
        // {
        //     print_transform(*transform);
        //     FL_LOG(sprite->TextureFilePath);
        // }
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
