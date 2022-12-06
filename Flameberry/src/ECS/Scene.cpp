#include "Scene.h"
#include "Core/Timer.h"

namespace Flameberry {
    Scene::Scene(Registry* registry)
        : m_Registry(registry)
    {}

    void Scene::RenderScene(OpenGLRenderer2D* renderer, const OrthographicCamera& camera)
    {
        renderer->Begin(camera);
        for (const auto& entity : m_Registry->View<TransformComponent, SpriteRendererComponent>())
        {
            const auto& [transform, sprite] = m_Registry->Get<TransformComponent, SpriteRendererComponent>(entity);
            if (sprite->TextureFilePath == "")
                renderer->AddQuad(transform->GetTransform(), sprite->Color, entity.get());
            else
                renderer->AddQuad(transform->GetTransform(), sprite->TextureFilePath.c_str(), entity.get());
        }
        renderer->End();
    }

    void Scene::RenderScene(OpenGLRenderer3D* renderer, const PerspectiveCamera& camera, const std::vector<PointLight>& lights)
    {
        renderer->Begin(camera);
        m_SceneData.ActiveSkybox.OnDraw(camera);
        for (const auto& entity : m_Registry->View<TransformComponent, MeshComponent>())
        {
            const auto& [transform, mesh] = m_Registry->Get<TransformComponent, MeshComponent>(entity);

            if (m_SceneData.Materials.find(mesh->MaterialName) != m_SceneData.Materials.end())
                m_SceneData.Meshes[mesh->MeshIndex].Draw(*transform, camera.GetPosition(), lights, m_SceneData.DirLight, m_SceneData.Materials[mesh->MaterialName], entity.get());
            else
                m_SceneData.Meshes[mesh->MeshIndex].Draw(*transform, camera.GetPosition(), lights, m_SceneData.DirLight, Material(), entity.get());
        }
        renderer->End();
    }

    void Scene::LoadMesh(const Mesh& mesh)
    {
        m_SceneData.Meshes.emplace_back(std::move(mesh));
    }
}
