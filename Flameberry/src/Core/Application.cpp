#include "Application.h"
#include <glad/glad.h>

#include "Core.h"
#include "Renderer/Renderer2D.h"
#include "Timer.h"
#include "ImGui/ImGuiLayer.h"
#include "ECS/Registry.h"
#include "ECS/SceneView.h"
#include "ECS/Component.h"

namespace Flameberry {
    void PrintTransform(TransformComponent& comp)
    {
        FL_LOG("Transform component is\nposition: {0}, {1}, {2}\nrotation: {3}, {4}, {5}\nscale: {6}, {7}, {8}",
            comp.translation.x,
            comp.translation.y,
            comp.translation.z,
            comp.rotation.x,
            comp.rotation.y,
            comp.rotation.z,
            comp.scale.x,
            comp.scale.y,
            comp.scale.z
        );
    }

    Application* Application::s_Instance;
    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();
        ImGuiLayer::OnAttach();

        Registry registry;
        Entity entity = registry.CreateEntity();
        TransformComponent* comp = registry.AddComponent<TransformComponent>(entity);
        comp->translation = glm::vec3(0.1f, 0, 0);
        comp->rotation = glm::vec3(45, 45, 90);
        comp->scale = glm::vec3(2, 1, 1);
        registry.AddComponent<SpriteRendererComponent>(entity)->TextureFilePath = "Hello";

        Entity entity1 = registry.CreateEntity();
        TransformComponent* comp1 = registry.AddComponent<TransformComponent>(entity1);
        comp1->translation = glm::vec3(0.2f, 0.3f, 0);
        comp1->rotation = glm::vec3(25, 25, 60);
        comp1->scale = glm::vec3(2, 2, 3);
        registry.AddComponent<SpriteRendererComponent>(entity1)->TextureFilePath = "Hi";

        Entity entity2 = registry.CreateEntity();
        TransformComponent* comp2 = registry.AddComponent<TransformComponent>(entity2);
        comp2->translation = glm::vec3(0.34f, 0.56f, 0);
        comp2->rotation = glm::vec3(65, 65, 20);
        comp2->scale = glm::vec3(3, 2, 1);
        registry.AddComponent<SpriteRendererComponent>(entity2)->TextureFilePath = "Comeon";

        Entity entity3 = registry.CreateEntity();
        TransformComponent* comp3 = registry.AddComponent<TransformComponent>(entity3);
        comp3->translation = glm::vec3(1, 2, 0);
        comp3->rotation = glm::vec3(100, 100, 600);
        comp3->scale = glm::vec3(8, 9, 10);
        registry.AddComponent<SpriteRendererComponent>(entity3)->TextureFilePath = "wow";

        for (auto [transform, sprite] : registry.View<TransformComponent, SpriteRendererComponent>())
        {
            PrintTransform(transform);
            FL_LOG(sprite.TextureFilePath);
        }

        // auto sceneView = registry.View<TransformComponent, SpriteRendererComponent>();
        // for (SceneView<TransformComponent, SpriteRendererComponent>::iterator it = sceneView.begin(); it != sceneView.end(); it++)
        // {
        //     FL_LOG(std::get<1>(*it).TextureFilePath);
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
