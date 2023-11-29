#pragma once

#include "Flameberry.h"
#include "MaterialEditorPanel.h"
#include "MaterialSelectorPanel.h"

namespace Flameberry {
    inline ImVec4 operator*(const ImVec4& a, const ImVec4& b) {
        return ImVec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
    }

    inline ImVec4 operator*(const ImVec4& a, float b) {
        return ImVec4(a.x * b, a.y * b, a.z * b, a.w * b);
    }

    class InspectorPanel
    {
    public:
        InspectorPanel();
        InspectorPanel(const std::shared_ptr<Scene>& context);
        ~InspectorPanel() = default;

        void SetContext(const std::shared_ptr<Scene>& context) { m_Context = context; }
        void SetSelectionContext(const fbentt::entity& selectionContext) { m_SelectionContext = selectionContext; }
        void OnUIRender();

        template<typename... Args>
        static std::shared_ptr<InspectorPanel> Create(Args... args) { return std::make_shared<InspectorPanel>(std::forward<Args>(args)...); }
    private:
        std::shared_ptr<fbentt::registry> GetContextRegistry() const { return m_Context->m_Registry; }
    private:
        static constexpr ImGuiTableFlags s_TableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_NoKeepColumnsVisible;
        static constexpr float s_LabelWidth = 100.0f;

        std::shared_ptr<MaterialEditorPanel> m_MaterialEditorPanel;
        std::shared_ptr<MaterialSelectorPanel> m_MaterialSelectorPanel;

        fbentt::entity m_SelectionContext = {};
        std::shared_ptr<Scene> m_Context;

        std::shared_ptr<Texture2D> m_SettingsIcon;
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
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 2, 4 });
            
            auto& style = ImGui::GetStyle();
            float lineHeight = ImGui::GetTextLineHeight() + 2.0f * style.FramePadding.y;
            
            bool open = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowOverlap);
            ImGui::PopStyleVar();
            
            float imageSize = lineHeight - 2.0f * style.FramePadding.y;
            ImGui::SameLine(contentRegionAvail.x - lineHeight * 0.5f);
            
            ImGui::ImageButton("ComponentSettingsToggle", reinterpret_cast<ImTextureID>(m_SettingsIcon->CreateOrGetDescriptorSet()), ImVec2(imageSize, imageSize));
            ImGui::PopStyleVar();
            
            bool shouldRemoveComp = false;
            if (ImGui::BeginPopupContextItem("ComponentSettings", ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonLeft))
            {
                ImGui::BeginDisabled(!removable);
                if (ImGui::MenuItem("Remove Component"))
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
