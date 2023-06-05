#include "SceneHierarchyPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "../Utils.h"

namespace Flameberry {
    SceneHierarchyPanel::SceneHierarchyPanel(const std::shared_ptr<Scene>& context)
        : m_Context(context),
        m_InspectorPanel(InspectorPanel::Create(m_Context))
    {
    }

    SceneHierarchyPanel::~SceneHierarchyPanel()
    {
    }

    void SceneHierarchyPanel::OnUIRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 5 });
        ImGui::Begin("Scene Hierarchy");
        ImGui::PopStyleVar();

        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::BeginMenu("Create"))
            {
                if (ImGui::MenuItem("Empty"))
                {
                    auto entity = m_Context->m_Registry->create();
                    m_Context->m_Registry->emplace<IDComponent>(entity);
                    m_Context->m_Registry->emplace<TagComponent>(entity).Tag = "Empty";
                    m_Context->m_Registry->emplace<TransformComponent>(entity);
                    m_SelectionContext = entity;
                }
                if (ImGui::MenuItem("Mesh"))
                {
                    auto entity = m_Context->m_Registry->create();
                    m_Context->m_Registry->emplace<IDComponent>(entity);
                    m_Context->m_Registry->emplace<TagComponent>(entity).Tag = "Mesh";
                    m_Context->m_Registry->emplace<TransformComponent>(entity);
                    m_Context->m_Registry->emplace<MeshComponent>(entity);
                    m_SelectionContext = entity;
                }
                if (ImGui::BeginMenu("Light"))
                {
                    if (ImGui::MenuItem("Point Light"))
                    {
                        auto entity = m_Context->m_Registry->create();
                        m_Context->m_Registry->emplace<IDComponent>(entity);
                        m_Context->m_Registry->emplace<TagComponent>(entity).Tag = "Point Light";
                        m_Context->m_Registry->emplace<TransformComponent>(entity);
                        m_Context->m_Registry->emplace<LightComponent>(entity);
                        m_SelectionContext = entity;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        m_Context->m_Registry->each([this](ecs::entity_handle& entity)
            {
                auto& tag = m_Context->m_Registry->get<TagComponent>(entity).Tag;

                // bool hasRelationShipComponent = m_ActiveScene->m_Registry->has<RelationshipComponent>(entity);
                // if (hasRelationShipComponent && m_ActiveScene->m_Registry->get<RelationshipComponent>(entity).Parent != ecs::entity_handle::null)
                //     return;

                bool is_renamed = m_RenamedEntity == entity;
                bool is_selected = m_SelectionContext == entity;
                bool should_delete_entity = false;
                int treeNodeFlags = (is_selected ? ImGuiTreeNodeFlags_Selected : 0)
                    | ImGuiTreeNodeFlags_OpenOnArrow
                    | ImGuiTreeNodeFlags_FramePadding
                    | ImGuiTreeNodeFlags_Leaf;

                if (m_RenamedEntity != entity)
                    treeNodeFlags |= ImGuiTreeNodeFlags_SpanFullWidth;

                ImGui::PushID((uint32_t)entity);

                float textColor = is_selected ? 0.0f : 1.0f;
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{ 1.0f, 197.0f / 255.0f, 86.0f / 255.0f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ textColor, textColor, textColor, 1.0f });

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 2.5f });
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
                bool open = ImGui::TreeNodeEx(is_renamed ? "" : tag.c_str(), treeNodeFlags);
                ImGui::PopStyleVar(2);
                if (open) {
                    ImGui::TreePop();
                }
                // if (open) {
                //     if (hasRelationShipComponent) {
                //         ecs::entity_handle iter_entity = entity;
                //         auto& relationship = m_ActiveScene->m_Registry->get<RelationshipComponent>(entity);
                //         while (relationship.FirstChild != ecs::entity_handle::null)
                //         {
                //             auto& childTag = m_ActiveScene->m_Registry->get<TagComponent>(relationship.FirstChild).Tag;
                //             if (ImGui::TreeNodeEx(is_renamed ? "" : childTag.c_str(), treeNodeFlags))
                //                 ImGui::TreePop();
                //             else break;

                //             if (m_ActiveScene->m_Registry->has<RelationshipComponent>(relationship.FirstChild))
                //                 relationship = m_ActiveScene->m_Registry->get<RelationshipComponent>(relationship.FirstChild);
                //         }
                //     }
                //     ImGui::TreePop();
                // }

                ImGui::PopStyleColor(is_selected ? 4 : 3);

                if (ImGui::IsItemClicked())
                    m_SelectionContext = entity;

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Rename"))
                        m_RenamedEntity = entity;

                    if (ImGui::MenuItem("Delete Entity"))
                        should_delete_entity = true;
                    ImGui::EndPopup();
                }

                if (is_renamed)
                    RenameNode(tag);

                if (should_delete_entity)
                {
                    m_Context->m_Registry->destroy(entity);
                    if (is_selected)
                    {
                        m_SelectionContext = ecs::entity_handle::null;
                    }
                }
                ImGui::PopID();
            }
        );

        ImGui::End();

        m_InspectorPanel->SetSelectionContext(m_SelectionContext);
        m_InspectorPanel->OnUIRender();
    }

    void SceneHierarchyPanel::RenameNode(std::string& tag)
    {
        memcpy(m_RenameBuffer, tag.c_str(), tag.size());

        ImGui::SameLine();
        ImGui::SetKeyboardFocusHere();
        ImGui::PushItemWidth(-1.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0, 0 });
        if (ImGui::InputText("###Rename", m_RenameBuffer, 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            tag = std::string(m_RenameBuffer);
            m_RenamedEntity = ecs::entity_handle::null;
        }
        ImGui::PopStyleVar();
    }
}
