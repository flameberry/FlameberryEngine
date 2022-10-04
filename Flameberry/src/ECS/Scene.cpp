#include "Scene.h"

namespace Flameberry {
    Scene::Scene(Registry* registry)
        : m_Registry(registry)
    {}

    void Scene::RenderScene(OpenGLRenderer2D* renderer, const OrthographicCamera& camera)
    {
        renderer->Begin(camera);
        for (const auto& entity : m_Registry->View<TransformComponent, SpriteRendererComponent>())
        {
            auto [transform, sprite] = m_Registry->Get<TransformComponent, SpriteRendererComponent>(entity);
            if (sprite->TextureFilePath == "")
                renderer->AddQuad(transform->GetTransform(), sprite->Color, entity.get());
            else
                renderer->AddQuad(transform->GetTransform(), sprite->TextureFilePath.c_str(), entity.get());
        }
        renderer->End();
    }
}
