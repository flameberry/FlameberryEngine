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

        if (ImGui::BeginPopupContextWindow((const char*)__null, m_PopupFlags))
        {
            DrawCreateEntityMenu();
            ImGui::EndPopup();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);

        m_IsSelectedNodeDisplayed = false;
        m_Context->m_Registry->each([this](fbentt::entity entity)
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

    void SceneHierarchyPanel::DrawEntityNode(fbentt::entity entity)
    {
        auto& tag = m_Context->m_Registry->get<TagComponent>(entity).Tag;

        bool is_renamed = m_RenamedEntity == entity;
        bool is_selected = m_SelectionContext == entity;
        m_IsSelectedNodeDisplayed = m_IsSelectedNodeDisplayed || is_selected;

        bool should_delete_entity = false;
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
            else if (io.KeySuper && ImGui::IsKeyPressed(ImGuiKey_Backspace))
                should_delete_entity = true;
            // else if (io.KeySuper && ImGui::IsKeyPressed(ImGuiKey_D)) // TOOD: Duplicate entity
                
        }

        if (ImGui::BeginPopupContextItem("EntityNodeContextMenu", m_PopupFlags))
        {
            if (ImGui::MenuItem("Rename"))
                m_RenamedEntity = entity;

            DrawCreateEntityMenu(entity);

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
                ReparentEntity(payloadEntity, entity);
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

        if (should_delete_entity)
            DestroyEntityTree(entity);
    }

    void SceneHierarchyPanel::DrawCreateEntityMenu(fbentt::entity parent)
    {
        if (ImGui::BeginMenu("Create"))
        {
            if (ImGui::MenuItem("Empty"))
            {
                auto entity = CreateEntityWithTagAndParent("Empty", parent);
                m_SelectionContext = entity;
            }
            if (ImGui::MenuItem("Mesh"))
            {
                auto entity = CreateEntityWithTagAndParent("StaticMesh", parent);
                m_Context->m_Registry->emplace<MeshComponent>(entity);
                m_SelectionContext = entity;
            }
            if (ImGui::BeginMenu("Light"))
            {
                if (ImGui::MenuItem("Point Light"))
                {
                    auto entity = CreateEntityWithTagAndParent("Point Light", parent);
                    m_Context->m_Registry->emplace<LightComponent>(entity);
                    m_SelectionContext = entity;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
    }

    fbentt::entity SceneHierarchyPanel::CreateEntityWithTagAndParent(const std::string& tag, fbentt::entity parent)
    {
        auto entity = m_Context->m_Registry->create();
        m_Context->m_Registry->emplace<IDComponent>(entity);
        m_Context->m_Registry->emplace<TagComponent>(entity).Tag = tag;
        m_Context->m_Registry->emplace<TransformComponent>(entity);

        if (parent != fbentt::null)
        {
            m_Context->m_Registry->emplace<RelationshipComponent>(entity);

            if (!m_Context->m_Registry->has<RelationshipComponent>(parent))
                m_Context->m_Registry->emplace<RelationshipComponent>(parent);

            auto& relation = m_Context->m_Registry->get<RelationshipComponent>(entity);
            relation.Parent = parent;

            auto& parentRel = m_Context->m_Registry->get<RelationshipComponent>(parent);
            if (parentRel.FirstChild == fbentt::null)
                parentRel.FirstChild = entity;
            else
            {
                auto sibling = parentRel.FirstChild;

                while (m_Context->m_Registry->get<RelationshipComponent>(sibling).NextSibling != fbentt::null)
                    sibling = m_Context->m_Registry->get<RelationshipComponent>(sibling).NextSibling;

                auto& siblingRel = m_Context->m_Registry->get<RelationshipComponent>(sibling);
                siblingRel.NextSibling = entity;
                relation.PrevSibling = sibling;
            }
        }
        return entity;
    }

    void SceneHierarchyPanel::DestroyEntityTree(fbentt::entity entity)
    {
        if (entity == fbentt::null)
            return;

        if (m_Context->m_Registry->has<RelationshipComponent>(entity))
        {
            auto& relation = m_Context->m_Registry->get<RelationshipComponent>(entity);
            auto sibling = relation.FirstChild;
            while (sibling != fbentt::null)
            {
                auto temp = m_Context->m_Registry->get<RelationshipComponent>(sibling).NextSibling;
                DestroyEntityTree(sibling);
                sibling = temp;
            }

            if (relation.Parent != fbentt::null)
            {
                auto& parentRel = m_Context->m_Registry->get<RelationshipComponent>(relation.Parent);
                if (parentRel.FirstChild == entity)
                    parentRel.FirstChild = relation.NextSibling;
            }
            if (relation.PrevSibling != fbentt::null)
                m_Context->m_Registry->get<RelationshipComponent>(relation.PrevSibling).NextSibling = relation.NextSibling;
            if (relation.NextSibling != fbentt::null)
                m_Context->m_Registry->get<RelationshipComponent>(relation.NextSibling).PrevSibling = relation.PrevSibling;
        }

        if (m_SelectionContext == entity)
            m_SelectionContext = fbentt::null;

        m_Context->m_Registry->destroy(entity);
    }

    void SceneHierarchyPanel::ReparentEntity(fbentt::entity entity, fbentt::entity parent)
    {
        if (IsEntityInHierarchy(parent, entity))
            return;

        if (!m_Context->m_Registry->has<RelationshipComponent>(entity))
            m_Context->m_Registry->emplace<RelationshipComponent>(entity);

        if (!m_Context->m_Registry->has<RelationshipComponent>(parent))
            m_Context->m_Registry->emplace<RelationshipComponent>(parent);

        auto& relation = m_Context->m_Registry->get<RelationshipComponent>(entity);

        auto oldParent = relation.Parent;
        if (oldParent != fbentt::null)
        {
            auto& oldParentRel = m_Context->m_Registry->get<RelationshipComponent>(oldParent);
            if (oldParentRel.FirstChild == entity)
                oldParentRel.FirstChild = relation.NextSibling;
        }
        if (relation.PrevSibling != fbentt::null)
            m_Context->m_Registry->get<RelationshipComponent>(relation.PrevSibling).NextSibling = relation.NextSibling;
        if (relation.NextSibling != fbentt::null)
            m_Context->m_Registry->get<RelationshipComponent>(relation.NextSibling).PrevSibling = relation.PrevSibling;

        auto& newParentRel = m_Context->m_Registry->get<RelationshipComponent>(parent);
        relation.NextSibling = newParentRel.FirstChild;
        relation.PrevSibling = fbentt::null;
        relation.Parent = parent;

        if (relation.NextSibling != fbentt::null)
            m_Context->m_Registry->get<RelationshipComponent>(relation.NextSibling).PrevSibling = entity;
        newParentRel.FirstChild = entity;
    }

    bool SceneHierarchyPanel::IsEntityInHierarchy(fbentt::entity key, fbentt::entity parent)
    {
        auto* relation = m_Context->m_Registry->try_get<RelationshipComponent>(parent);
        auto sibling = parent;
        while (relation && sibling != fbentt::null)
        {
            if (sibling == key)
                return true;

            if (relation->FirstChild != fbentt::null && IsEntityInHierarchy(key, relation->FirstChild))
                return true;

            sibling = relation->NextSibling;
            relation = m_Context->m_Registry->try_get<RelationshipComponent>(sibling);
        }
        return false;
    }
}
