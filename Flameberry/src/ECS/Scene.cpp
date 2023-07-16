#include "Scene.h"

#include "Core/Timer.h"
#include "Components.h"

namespace Flameberry {
    Scene::Scene()
        : m_Registry(std::make_shared<fbentt::registry>())
    {
    }

    Scene::Scene(const std::shared_ptr<Scene>& other)
        : m_Registry(std::make_shared<fbentt::registry>(*other->m_Registry)), m_Name(other->m_Name), m_Environment(other->m_Environment)
    {
        FL_LOG("Copying Scene...");
    }

    Scene::~Scene()
    {
        FL_LOG("Deleting Scene...");
    }

    void Scene::OnUpdateRuntime(float delta)
    {
        for (auto entity : m_Registry->view<TransformComponent>()) {
            m_Registry->get<TransformComponent>(entity).rotation.y += 2.0f * delta;
        }
    }

    void Scene::RenderScene(const glm::mat4& cameraMatrix)
    {
    }
}
