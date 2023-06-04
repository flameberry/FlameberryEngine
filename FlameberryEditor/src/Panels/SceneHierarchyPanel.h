#pragma once

#include "Flameberry.h"
#include "MaterialSelectorPanel.h"
#include "MaterialEditorPanel.h"

namespace Flameberry {
    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel(Flameberry::Scene* scene = nullptr);
        ~SceneHierarchyPanel();
        void OnUIRender();
        void SetSelectedEntity(const ecs::entity_handle& entity) { m_SelectedEntity = entity; }
        void RenameNode(std::string& tag);
        ecs::entity_handle GetSelectedEntity() const { return m_SelectedEntity; }

        void OnEnvironmentMapPanelRender();
    private:
        void DrawComponent(TransformComponent& transform);
        void DrawComponent(MeshComponent& mesh);
        void DrawComponent(LightComponent& light);
    private:
        ecs::entity_handle m_SelectedEntity, m_RenamedEntity;
        Scene* m_ActiveScene;

        ImGuiTableFlags m_TableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoKeepColumnsVisible;

        VkSampler m_VkTextureSampler;
        VulkanTexture m_PlusIconTexture, m_MinusIconTexture;

        std::shared_ptr<MaterialSelectorPanel> m_MaterialSelectorPanel;
        std::shared_ptr<MaterialEditorPanel> m_MaterialEditorPanel;

        char m_RenameBuffer[256];
    };
}