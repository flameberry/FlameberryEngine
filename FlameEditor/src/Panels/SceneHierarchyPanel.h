#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel(Flameberry::Scene* scene = nullptr);
        ~SceneHierarchyPanel() = default;
        void OnUIRender();
        void SetSelectedEntity(const Flameberry::entity_handle& entity) { m_SelectedEntity = entity; }
        std::string RenameNode(const char* name);
    private:
        void DrawComponent(Flameberry::TransformComponent& transform);
        void DrawComponent(Flameberry::SpriteRendererComponent& sprite);
        void DrawComponent(Flameberry::MeshComponent& mesh);
    private:
        Flameberry::entity_handle m_SelectedEntity;
        Flameberry::Scene* m_ActiveScene;
    private:
        uint32_t m_DefaultTextureId;
    };
}