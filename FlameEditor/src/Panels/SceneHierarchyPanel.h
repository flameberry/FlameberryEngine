#pragma once

#include "Flameberry.h"

class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel(Flameberry::Scene* scene = nullptr, std::vector<Flameberry::Mesh>* meshes = nullptr);
    ~SceneHierarchyPanel() = default;
    void OnUIRender();
    void SetSelectedEntity(const Flameberry::entity_handle& entity) { m_SelectedEntity = entity; }
private:
    void DrawComponent(Flameberry::TransformComponent& transform);
    void DrawComponent(Flameberry::SpriteRendererComponent& sprite);
    void DrawComponent(Flameberry::MeshComponent& mesh);
private:
    Flameberry::entity_handle m_SelectedEntity;
    Flameberry::Scene* m_Scene;
    std::vector<Flameberry::Mesh>* m_Meshes;
private:
    uint32_t m_DefaultTextureId;
};
