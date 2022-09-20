#include "Scene.h"

namespace Flameberry {
    Scene::Scene(Registry* registry)
        : m_Registry(registry)
    {}

    void Scene::RenderScene(Renderer2D* renderer, const OrthographicCamera& camera)
    {
        renderer->Begin(camera);

        auto sceneView = m_Registry->View<TransformComponent, SpriteRendererComponent>();
        for (SceneView<TransformComponent, SpriteRendererComponent>::iterator it = sceneView.begin(); it != sceneView.end(); it++)
        {
            if (std::get<1>(*it)->TextureFilePath == "")
                renderer->AddQuad(std::get<0>(*it)->GetTransform(), std::get<1>(*it)->Color);
            else
                renderer->AddQuad(std::get<0>(*it)->GetTransform(), std::get<1>(*it)->TextureFilePath.c_str());
        }
        renderer->End();
    }
}