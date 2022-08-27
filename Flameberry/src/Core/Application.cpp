#include "Application.h"
#include <glad/glad.h>

#include "Core.h"
#include "Renderer/Renderer2D.h"
#include "Timer.h"
#include "ImGui/ImGuiLayer.h"
#include "ECS/Scene.h"
#include "ECS/Component.h"

namespace Flameberry {
    Application* Application::s_Instance;
    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();
        ImGuiLayer::OnAttach();

        Scene scene;
        Entity entity = scene.CreateEntity();
        scene.AddComponent<TransformComponent>(entity);
        TransformComponent* transformComponent = scene.GetComponent<TransformComponent>(entity);
        transformComponent->position = glm::vec3(0, 0.1f, 0);
        transformComponent->rotation = glm::vec3(45, 0, 90);
        transformComponent->scale = glm::vec3(1, 1, 2);

        FL_LOG("Transform component of the entity with id: {0}, is\nposition: {1}, {2}, {3}\nrotation: {4}, {5}, {6}\nscale: {7}, {8}, {9}",
            entity.entityId,
            transformComponent->position.x,
            transformComponent->position.y,
            transformComponent->position.z,
            transformComponent->rotation.x,
            transformComponent->rotation.y,
            transformComponent->rotation.z,
            transformComponent->scale.x,
            transformComponent->scale.y,
            transformComponent->scale.z
        );

        FL_LOG("Entity is: {0}", entity.GetValidity() ? "valid" : "not valid");
        scene.DestroyEntity(entity);
        FL_LOG("Entity is: {0}", entity.GetValidity() ? "valid" : "not valid");
        Entity entityNew = scene.CreateEntity();
        scene.AddComponent<TransformComponent>(entity);
        TransformComponent* someTransformComponent = scene.GetComponent<TransformComponent>(entity);
        scene.AddComponent<TransformComponent>(entityNew);
        TransformComponent* newTransformComponent = scene.GetComponent<TransformComponent>(entityNew);
        newTransformComponent->position = glm::vec3(0, 0.1f, 0);
        newTransformComponent->rotation = glm::vec3(45, 0, 90);
        newTransformComponent->scale = glm::vec3(1, 1, 2);

        FL_LOG("Transform component of the new entity with id: {0}, is\nposition: {1}, {2}, {3}\nrotation: {4}, {5}, {6}\nscale: {7}, {8}, {9}",
            entityNew.entityId,
            newTransformComponent->position.x,
            newTransformComponent->position.y,
            newTransformComponent->position.z,
            newTransformComponent->rotation.x,
            newTransformComponent->rotation.y,
            newTransformComponent->rotation.z,
            newTransformComponent->scale.x,
            newTransformComponent->scale.y,
            newTransformComponent->scale.z
        );
        scene.RemoveComponent<TransformComponent>(entityNew);
        scene.RemoveComponent<TransformComponent>(entityNew);
        scene.RemoveComponent<TransformComponent>(entity);
        scene.GetComponent<TransformComponent>(entityNew);
        scene.DestroyEntity(entityNew);
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
