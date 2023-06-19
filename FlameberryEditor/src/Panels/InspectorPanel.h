#pragma once

#include "Flameberry.h"
#include "MaterialEditorPanel.h"
#include "MaterialSelectorPanel.h"

namespace Flameberry {
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
        ImGuiTableFlags m_TableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoKeepColumnsVisible | ImGuiTableFlags_PadOuterX;

        std::shared_ptr<MaterialEditorPanel> m_MaterialEditorPanel;
        std::shared_ptr<MaterialSelectorPanel> m_MaterialSelectorPanel;

        fbentt::entity m_SelectionContext = {};
        std::shared_ptr<Scene> m_Context;

        std::shared_ptr<Texture2D> m_TripleDotsIcon;

        template<typename ComponentType, typename Fn>
        friend void DrawComponent(const char* name, InspectorPanel* instance, Fn&& fn);
    };

    template<typename ComponentType, typename Fn>
    void DrawComponent(const char* name, InspectorPanel* instance, Fn&& fn)
    {
        static_assert(std::is_invocable_v<Fn>);
        if (instance->GetContextRegistry()->has<ComponentType>(instance->m_SelectionContext))
        {
            ImGui::PushID(fbentt::type_id<ComponentType>());

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 4.0f });
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            bool open = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap);
            ImGui::PopStyleVar(2);

            float width = 16.0f;
            ImGui::SameLine(ImGui::GetWindowSize().x - width - ImGui::GetStyle().ItemSpacing.x);
            ImGui::ImageButton(reinterpret_cast<ImTextureID>(instance->m_TripleDotsIcon->GetDescriptorSet()), ImVec2(width, width));

            bool should_remove_comp = false;
            if (ImGui::BeginPopupContextItem("ComponentSettings", ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("Remove Component"))
                    should_remove_comp = true;
                ImGui::EndPopup();
            }

            if (open)
                fn();

            ImGui::PopID();

            if (should_remove_comp)
                instance->GetContextRegistry()->erase<ComponentType>(instance->m_SelectionContext);
        }
    }
}
