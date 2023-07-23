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
        void SetContext(const std::shared_ptr<Scene>& context);
        void SetSelectionContext(fbentt::entity entity);
        
        fbentt::entity GetSelectionContext() const { return m_SelectionContext; }
        bool IsFocused() const { return m_IsFocused; }

        void DrawEntityNode(fbentt::entity entity);
        void DrawCreateEntityMenu(fbentt::entity parent = (fbentt::entity)fbentt::null);
        
        template<typename... Args>
        static std::shared_ptr<SceneHierarchyPanel> Create(Args... args) { return std::make_shared<SceneHierarchyPanel>(std::forward<Args>(args)...); }
    private:
        fbentt::entity m_SelectionContext = {}, m_RenamedEntity = {};
        std::shared_ptr<Scene> m_Context;
        std::shared_ptr<InspectorPanel> m_InspectorPanel;

        ImGuiPopupFlags m_PopupFlags = ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight;
        char m_RenameBuffer[256];

        bool m_IsSelectedNodeDisplayed = false, m_IsFocused = false;
    };
}
