#include "SceneHierarchyPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "ContentBrowserPanel.h"

SceneHierarchyPanel::SceneHierarchyPanel(Flameberry::Scene* scene)
    : m_Scene(scene), m_SelectedEntity(UINT64_MAX, false)
{
    m_DefaultTextureId = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/Checkerboard.png");
}

void SceneHierarchyPanel::OnUIRender()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 5 });
    ImGui::Begin("Scene Hierarchy");
    ImGui::PopStyleVar();

    if (ImGui::BeginPopupContextWindow())
    {
        if (ImGui::MenuItem("Create Empty"))
        {
            Flameberry::entity_handle entity = m_Scene->GetRegistry()->CreateEntity();
            m_Scene->GetRegistry()->AddComponent<Flameberry::TagComponent>(entity)->Tag = "Empty";
            m_Scene->GetRegistry()->AddComponent<Flameberry::TransformComponent>(entity);
        }
        ImGui::EndPopup();
    }

    m_Scene->GetRegistry()->each([this](Flameberry::entity_handle& entity)
        {
            auto& tag = m_Scene->GetRegistry()->GetComponent<Flameberry::TagComponent>(entity)->Tag;
            int treeNodeFlags = (m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            ImGui::PushID(entity.get());

            if (ImGui::TreeNodeEx(tag.c_str(), treeNodeFlags))
                ImGui::TreePop();

            if (ImGui::IsItemClicked())
                m_SelectedEntity = entity;

            bool should_delete_entity = false;
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Rename"));
                if (ImGui::MenuItem("Delete Entity"))
                    should_delete_entity = true;
                ImGui::EndPopup();
            }

            if (should_delete_entity)
                m_Scene->GetRegistry()->DestroyEntity(entity);

            ImGui::PopID();
        });

    ImGui::End();

    ImGui::Begin("Inspector");
    if (m_SelectedEntity.is_valid())
    {
        if (m_Scene->GetRegistry()->HasComponent<Flameberry::TransformComponent>(m_SelectedEntity))
        {
            auto& transform = *m_Scene->GetRegistry()->GetComponent<Flameberry::TransformComponent>(m_SelectedEntity);
            DrawComponent(transform);
            ImGui::NewLine();
        }

        if (m_Scene->GetRegistry()->HasComponent<Flameberry::SpriteRendererComponent>(m_SelectedEntity))
        {
            auto& sprite = *m_Scene->GetRegistry()->GetComponent<Flameberry::SpriteRendererComponent>(m_SelectedEntity);
            DrawComponent(sprite);
        }

        if (ImGui::BeginPopupContextWindow((const char*)__null, ImGuiMouseButton_Right, false))
        {
            if (ImGui::MenuItem("Transform Component"))
                m_Scene->GetRegistry()->AddComponent<Flameberry::TransformComponent>(m_SelectedEntity);
            if (ImGui::MenuItem("Sprite Renderer Component"))
                m_Scene->GetRegistry()->AddComponent<Flameberry::SpriteRendererComponent>(m_SelectedEntity);
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void SceneHierarchyPanel::DrawVec3Control(const std::string& label, glm::vec3& value, float defaultValue, float dragSpeed)
{
    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, 70.0f);
    ImGui::Text("%s", label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    if (ImGui::Button("X", buttonSize))
        value.x = defaultValue;
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &value.x, dragSpeed, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    if (ImGui::Button("Y", buttonSize))
        value.y = defaultValue;
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &value.y, dragSpeed, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    if (ImGui::Button("Z", buttonSize))
        value.z = defaultValue;
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &value.z, dragSpeed, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();

    ImGui::Columns(1);

    ImGui::PopID();
}

void SceneHierarchyPanel::DrawComponent(Flameberry::TransformComponent& transform)
{
    DrawVec3Control("Translation", transform.translation, 0.0f, 0.01f);
    ImGui::Spacing();
    DrawVec3Control("Rotation", transform.rotation, 0.0f, 0.01f);
    ImGui::Spacing();
    DrawVec3Control("Scale", transform.scale, 1.0f, 0.01f);
}

void SceneHierarchyPanel::DrawComponent(Flameberry::SpriteRendererComponent& sprite)
{
    ImGui::ColorEdit4("Color", glm::value_ptr(sprite.Color));

    ImGui::Button("Texture", ImVec2{ ImGui::GetContentRegionAvail().x, 30 });
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_FILE_PATH"))
        {
            std::string path = (const char*)payload->Data;
            // memcpy(path, payload->Data, payload->DataSize);
            // const char* path = (const char*)payload->Data;

            std::filesystem::path texturePath{ path };
            texturePath = ContentBrowserPanel::s_SourceDirectory / texturePath;
            const std::string& ext = texturePath.extension().string();

            FL_LOG("Payload recieved: {0}, with extension {1}", path, ext);

            if (std::filesystem::exists(texturePath) && std::filesystem::is_regular_file(texturePath) && (ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
                sprite.TextureFilePath = texturePath.string();
            else
                FL_WARN("Bad File given as texture!");
        }
        ImGui::EndDragDropTarget();
    }

    uint32_t textureId;
    if (sprite.TextureFilePath == "")
        textureId = m_DefaultTextureId;
    else
        textureId = Flameberry::OpenGLRenderCommand::CreateTexture(sprite.TextureFilePath);
    ImGui::Image(reinterpret_cast<ImTextureID>(textureId), ImVec2{ 50, 50 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
}
