#pragma once

#include "Flameberry.h"
#include "InspectorPanel.h"

namespace Flameberry {
    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel(const std::shared_ptr<Scene>& context);
        ~SceneHierarchyPanel();

        void OnUIRender();

        void RenameNode(std::string& tag);
        void SetSelectionContext(const ecs::entity_handle& entity) { m_SelectionContext = entity; }
        ecs::entity_handle GetSelectionContext() const { return m_SelectionContext; }

        template<typename... Args>
        static std::shared_ptr<SceneHierarchyPanel> Create(Args... args) { return std::make_shared<SceneHierarchyPanel>(std::forward<Args>(args)...); }
    private:
        ecs::entity_handle m_SelectionContext = {}, m_RenamedEntity = {};
        std::shared_ptr<Scene> m_Context;
        std::shared_ptr<InspectorPanel> m_InspectorPanel;

        ImGuiTableFlags m_TableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoKeepColumnsVisible;
        char m_RenameBuffer[256];
    };
}