#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel(Flameberry::Scene* scene = nullptr);
        ~SceneHierarchyPanel() = default;
        void OnUIRender();
        void SetSelectedEntity(const ecs::entity_handle& entity) { m_SelectedEntity = entity; }
        std::string RenameNode(const char* name);
        ecs::entity_handle GetSelectedEntity() const { return m_SelectedEntity; }
    private:
        void DrawComponent(TransformComponent& transform);
        void DrawComponent(SpriteRendererComponent& sprite);
        void DrawComponent(MeshComponent& mesh);
        void DrawComponent(LightComponent& light);
    private:
        ecs::entity_handle m_SelectedEntity;
        Scene* m_ActiveScene;
    private:
        uint32_t m_DefaultTextureId;
    };
}