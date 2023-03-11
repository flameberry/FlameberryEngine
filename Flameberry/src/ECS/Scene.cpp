#include "Scene.h"
#include "Core/Timer.h"

namespace Flameberry {
    Scene::Scene(ecs::registry* registry)
        : m_Registry(registry)
    {}

    void Scene::LoadMesh(const Mesh& mesh)
    {
        m_SceneData.Meshes.emplace_back(std::move(mesh));
    }
}
