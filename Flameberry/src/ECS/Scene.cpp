#include "Scene.h"
#include "Core/Timer.h"

namespace Flameberry {
    Scene::Scene(ecs::registry* registry)
        : m_Registry(registry)
    {}

    void Scene::LoadMesh(const char* filePath)
    {
        m_SceneData.Meshes.emplace_back(std::make_shared<VulkanMesh>(filePath));
    }
}
