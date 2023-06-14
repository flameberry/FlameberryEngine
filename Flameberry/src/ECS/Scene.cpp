#include "Scene.h"
#include "Core/Timer.h"

namespace Flameberry {
    Scene::Scene(const std::shared_ptr<fbentt::registry>& registry)
        : m_Registry(registry)
    {}

    void Scene::RenderScene(const glm::mat4& cameraMatrix)
    {
    }
}
