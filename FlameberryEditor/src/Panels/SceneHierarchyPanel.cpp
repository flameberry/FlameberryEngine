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
        
        m_IsFocused = ImGui::IsWindowFocused();

        if (ImGui::BeginPopupContextWindow((const char*)__null, m_PopupFlags))
        {
            DrawCreateEntityMenu();
            ImGui::EndPopup();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);

        m_IsSelectedNodeDisplayed = false;
        m_Context->m_Registry->for_each([this](fbentt::entity entity)
            {
                auto* relation = m_Context->m_Registry->try_get<RelationshipComponent>(entity);
                if (!relation || relation->Parent == fbentt::null)
                    DrawEntityNode(entity);
            }
        );

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
            m_SelectionContext = fbentt::null;

        ImGui::PopStyleVar();
        ImGui::End();

        m_InspectorPanel->SetSelectionContext(m_SelectionContext);
        m_InspectorPanel->OnUIRender();
    }

    void SceneHierarchyPanel::RenameNode(std::string& tag)
    {
        strcpy(m_RenameBuffer, tag.c_str());

        ImGui::SameLine();
        ImGui::SetKeyboardFocusHere();
        ImGui::PushItemWidth(-1.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 2.5f });
        if (ImGui::InputText("###Rename", m_RenameBuffer, 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            tag = std::string(m_RenameBuffer);
            m_RenamedEntity = fbentt::null;
        }
        ImGui::PopStyleVar();
        ImGui::PopItemWidth();
    }

    void SceneHierarchyPanel::SetContext(const std::shared_ptr<Scene>& context)
    {
        m_Context = context;
        m_InspectorPanel->SetContext(m_Context);
    }

    void SceneHierarchyPanel::SetSelectionContext(fbentt::entity entity)
    {
        m_SelectionContext = entity;
        m_InspectorPanel->SetSelectionContext(m_SelectionContext);
    }

    void SceneHierarchyPanel::DrawEntityNode(fbentt::entity entity)
    {
        auto& tag = m_Context->m_Registry->get<TagComponent>(entity).Tag;

        bool is_renamed = m_RenamedEntity == entity;
        bool is_selected = m_SelectionContext == entity;
        m_IsSelectedNodeDisplayed = m_IsSelectedNodeDisplayed || is_selected;

        bool should_delete_entity = false, should_duplicate_entity = false;
        int treeNodeFlags = (is_selected ? ImGuiTreeNodeFlags_Selected : 0)
            | ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_FramePadding;

        bool has_child = false;

        if (m_Context->m_Registry->has<RelationshipComponent>(entity))
        {
            auto& relation = m_Context->m_Registry->get<RelationshipComponent>(entity);
            has_child = relation.FirstChild != fbentt::null;
        }

        if (!has_child)
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;

        if (m_RenamedEntity != entity)
            treeNodeFlags |= ImGuiTreeNodeFlags_SpanFullWidth;

        ImGui::PushID((const void*)(uint64_t)entity);

        float textColor = is_selected ? 0.0f : 1.0f;
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{ 1.0f, 197.0f / 255.0f, 86.0f / 255.0f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
        if (is_selected)
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ textColor, textColor, textColor, 1.0f });

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 2.5f });
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

        if (!m_IsSelectedNodeDisplayed) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        }

        bool open = ImGui::TreeNodeEx((const void*)(uint64_t)entity, treeNodeFlags, "%s", tag.c_str());

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(is_selected ? 4 : 3);

        if (ImGui::IsItemClicked())
            m_SelectionContext = entity;

        if (is_selected && ImGui::IsWindowFocused())
        {
            ImGuiIO& io = ImGui::GetIO();
            if (!io.KeyMods && ImGui::IsKeyPressed(ImGuiKey_Enter)) // TODO: Shouldn't work with modifier but it does
                m_RenamedEntity = entity;
        }

        if (ImGui::BeginPopupContextItem("EntityNodeContextMenu", m_PopupFlags))
        {
            if (ImGui::MenuItem("Rename"))
                m_RenamedEntity = entity;

            DrawCreateEntityMenu(entity);

            if (ImGui::MenuItem("Duplicate Entity"))
                should_duplicate_entity = true;

            if (ImGui::MenuItem("Delete Entity"))
                should_delete_entity = true;

            ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(
                "FL_SCENE_HIERARCHY_ENTITY_NODE",
                &entity,
                sizeof(entity),
                ImGuiCond_Once
            );
            ImGui::Text("%s", tag.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_SCENE_HIERARCHY_ENTITY_NODE"))
            {
                const fbentt::entity payloadEntity = *((const fbentt::entity*)payload->Data);
                m_Context->ReparentEntity(payloadEntity, entity);
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();

        if (is_renamed)
            RenameNode(tag);

        if (open) {
            if (has_child) {
                auto child = m_Context->m_Registry->get<RelationshipComponent>(entity).FirstChild;
                while (child != fbentt::null)
                {
                    auto temp = m_Context->m_Registry->get<RelationshipComponent>(child).NextSibling;
                    DrawEntityNode(child);
                    child = temp;
                }
            }
            ImGui::TreePop();
        }

        if (should_duplicate_entity)
        {
            const auto duplicate = m_Context->DuplicateEntity(entity);
            m_SelectionContext = duplicate;
        }

        if (should_delete_entity)
        {
            m_Context->DestroyEntityTree(entity);
            if (m_SelectionContext == entity)
                m_SelectionContext = fbentt::null;
        }
    }

    void SceneHierarchyPanel::DrawCreateEntityMenu(fbentt::entity parent)
    {
        if (ImGui::BeginMenu("Create"))
        {
            if (ImGui::MenuItem("Empty"))
            {
                auto entity = m_Context->CreateEntityWithTagAndParent("Empty", parent);
                m_SelectionContext = entity;
            }
            if (ImGui::MenuItem("Mesh"))
            {
                auto entity = m_Context->CreateEntityWithTagAndParent("StaticMesh", parent);
                m_Context->m_Registry->emplace<MeshComponent>(entity);
                m_SelectionContext = entity;
            }
            if (ImGui::BeginMenu("Light"))
            {
                if (ImGui::MenuItem("Point Light"))
                {
                    auto entity = m_Context->CreateEntityWithTagAndParent("Point Light", parent);
                    m_Context->m_Registry->emplace<LightComponent>(entity);
                    m_SelectionContext = entity;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
    }
}
