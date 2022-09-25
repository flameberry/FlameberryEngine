#include "SceneHierarchyPanel.h"

#include <glm/gtc/type_ptr.hpp>

SceneHierarchyPanel::SceneHierarchyPanel(Flameberry::Scene* scene)
    : m_Scene(scene), m_SelectedEntity(UINT64_MAX, false)
{
}

void SceneHierarchyPanel::OnUIRender()
{
    ImGui::Begin("Scene Hierarchy");
    for (const auto& entity : m_Scene->GetRegistry()->GetEntityVector())
    {
        if (entity.is_valid())
        {
            const auto& tag = m_Scene->GetRegistry()->GetComponent<Flameberry::TagComponent>(entity)->Tag;
            int treeNodeFlags = (m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            ImGui::PushID(entity.get());

            if (ImGui::TreeNodeEx(tag.c_str(), treeNodeFlags))
                ImGui::TreePop();

            if (ImGui::IsItemClicked())
                m_SelectedEntity = entity;

            ImGui::PopID();
        }
    }
    ImGui::End();

    ImGui::Begin("Inspector");
    if (m_SelectedEntity.is_valid())
    {
        auto& transform = *m_Scene->GetRegistry()->GetComponent<Flameberry::TransformComponent>(m_SelectedEntity);
        DrawComponent(transform);
        ImGui::NewLine();
        auto& sprite = *m_Scene->GetRegistry()->GetComponent<Flameberry::SpriteRendererComponent>(m_SelectedEntity);
        DrawComponent(sprite);
    }
    ImGui::End();
}

void SceneHierarchyPanel::DrawComponent(Flameberry::TransformComponent& transform)
{
    DrawVec3Control("Translation", transform.translation, 0.0f, 0.01f);
    ImGui::Spacing();
    DrawVec3Control("Rotation", transform.rotation, 0.0f, 0.01f);
    ImGui::Spacing();
    DrawVec3Control("Scale", transform.scale, 1.0f, 0.01f);
}

void SceneHierarchyPanel::DrawVec3Control(const std::string& label, glm::vec3& value, float defaultValue, float dragSpeed)
{
    ImGui::PushID(label.c_str());
    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

    ImGui::Text(label.c_str());
    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
    if (ImGui::Button("X", buttonSize))
        value.x = defaultValue;
    ImGui::SameLine();
    ImGui::DragFloat("##X", &value.x, dragSpeed, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    if (ImGui::Button("Y", buttonSize))
        value.y = defaultValue;
    ImGui::SameLine();
    ImGui::DragFloat("##Y", &value.y, dragSpeed, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    if (ImGui::Button("Z", buttonSize))
        value.z = defaultValue;
    ImGui::SameLine();
    ImGui::DragFloat("##Z", &value.z, dragSpeed, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
    ImGui::PopID();
}

void SceneHierarchyPanel::DrawComponent(Flameberry::SpriteRendererComponent& sprite)
{
    char hello[100] = "Hello";
    ImGui::ColorEdit4("Albedo", glm::value_ptr(sprite.Color));
    ImGui::Spacing();
    ImGui::InputText("Texture Path", hello, sizeof(hello));
}
