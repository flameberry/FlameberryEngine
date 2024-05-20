#pragma once

#include <IconFontCppHeaders/IconsLucide.h>

#include "Flameberry.h"
#include "MaterialEditorPanel.h"
#include "MaterialSelectorPanel.h"

namespace Flameberry {

    class InspectorPanel
    {
    public:
        InspectorPanel();
        InspectorPanel(const Ref<Scene>& context);
        ~InspectorPanel() = default;

        void SetContext(const Ref<Scene>& context) { m_Context = context; }
        void SetSelectionContext(const fbentt::entity& selectionContext) { m_SelectionContext = selectionContext; }
        void OnUIRender();
    private:
        Ref<fbentt::registry> GetContextRegistry() const { return m_Context->m_Registry; }
    private:
        static constexpr ImGuiTableFlags s_TableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoKeepColumnsVisible | ImGuiTableFlags_PadOuterX;
        static constexpr float s_LabelWidth = 100.0f;

        Ref<MaterialEditorPanel> m_MaterialEditorPanel;
        Ref<MaterialSelectorPanel> m_MaterialSelectorPanel;

        fbentt::entity m_SelectionContext = {};
        Ref<Scene> m_Context;

        Ref<Texture2D> m_SettingsIcon;
    private:
        template<typename ComponentType>
        void DrawAddComponentEntry(const char* name);

        template<typename ComponentType, typename Fn>
        void DrawComponent(const char* name, Fn&& fn, bool removable = true);
    };

    template<typename ComponentType>
    void InspectorPanel::DrawAddComponentEntry(const char* name)
    {
        if (!m_Context->m_Registry->has<ComponentType>(m_SelectionContext))
        {
            if (ImGui::MenuItem(name))
                m_Context->m_Registry->emplace<ComponentType>(m_SelectionContext);
        }
    }

    template<typename ComponentType, typename Fn>
    void InspectorPanel::DrawComponent(const char* name, Fn&& fn, bool removable)
    {
        static_assert(std::is_invocable_v<Fn>);
        if (m_Context->m_Registry->has<ComponentType>(m_SelectionContext))
        {
            ImGui::PushID(name);

            ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 4));

            auto& style = ImGui::GetStyle();
            float lineHeight = ImGui::GetTextLineHeight() + 2.0f * style.FramePadding.y;

            bool open = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowOverlap);
            ImGui::PopStyleVar();

            ImGui::SameLine(contentRegionAvail.x - lineHeight);
            ImGui::Button(ICON_LC_SETTINGS, ImVec2(0.0f, lineHeight));
            ImGui::PopStyleVar();

            bool shouldRemoveComp = false;
            if (ImGui::BeginPopupContextItem("ComponentSettings", ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonLeft))
            {
                ImGui::BeginDisabled(!removable);
                if (ImGui::MenuItem(ICON_LC_DELETE"\tRemove Component"))
                    shouldRemoveComp = true;
                ImGui::EndDisabled();

                ImGui::EndPopup();
            }

            if (open)
                fn();
            ImGui::PopID();

            if (shouldRemoveComp)
                m_Context->m_Registry->erase<ComponentType>(m_SelectionContext);
        }
    }

}
