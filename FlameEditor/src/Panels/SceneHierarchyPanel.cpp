#include "SceneHierarchyPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "../project_globals.h"

SceneHierarchyPanel::SceneHierarchyPanel(Flameberry::Scene* scene)
    : m_Scene(scene), m_SelectedEntity(UINT32_MAX, false)
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
            bool should_delete_entity = false;
            static Flameberry::entity_handle* entity_to_be_renamed = nullptr;

            auto& tag = m_Scene->GetRegistry()->GetComponent<Flameberry::TagComponent>(entity)->Tag;

            bool is_selected = m_SelectedEntity == entity;
            int treeNodeFlags = (is_selected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
            if (!entity_to_be_renamed || entity != *entity_to_be_renamed)
                treeNodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;

            ImGui::PushID(entity.get());

            float textColor = is_selected ? 0.0f : 1.0f;
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{ 1.0f, 197.0f / 255.0f, 86.0f / 255.0f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ textColor, textColor, textColor, 1.0f });

            if (ImGui::TreeNodeEx(tag.c_str(), treeNodeFlags))
                ImGui::TreePop();

            ImGui::PopStyleColor(is_selected ? 4 : 3);

            if (ImGui::IsItemClicked())
                m_SelectedEntity = entity;

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Rename"))
                    entity_to_be_renamed = &entity;

                if (ImGui::MenuItem("Delete Entity"))
                    should_delete_entity = true;
                ImGui::EndPopup();
            }

            if (entity_to_be_renamed && *entity_to_be_renamed == entity)
            {
                std::string buffer(tag);

                ImGui::SameLine();
                ImGui::SetKeyboardFocusHere();
                ImGui::PushItemWidth(-1.0f);

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0, 0 });
                if (ImGui::InputText("###Rename", buffer.data(), 100, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    tag = buffer;
                    entity_to_be_renamed = nullptr;
                }
                ImGui::PopStyleVar();
            }

            if (should_delete_entity)
                m_Scene->GetRegistry()->DestroyEntity(entity);

            if (m_SelectedEntity == entity)
                m_Scene->SetSelectedEntity(&entity);

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
            ImGui::Separator();
        }

        if (m_Scene->GetRegistry()->HasComponent<Flameberry::SpriteRendererComponent>(m_SelectedEntity))
        {
            ImGui::Spacing();
            auto& sprite = *m_Scene->GetRegistry()->GetComponent<Flameberry::SpriteRendererComponent>(m_SelectedEntity);
            DrawComponent(sprite);
            ImGui::Spacing();
            ImGui::Separator();
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

    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0];

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

    ImGui::PushFont(boldFont);
    if (ImGui::Button("X", buttonSize))
        value.x = defaultValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &value.x, dragSpeed, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });

    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y", buttonSize))
        value.y = defaultValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &value.y, dragSpeed, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });

    ImGui::PushFont(boldFont);
    if (ImGui::Button("Z", buttonSize))
        value.z = defaultValue;
    ImGui::PopFont();
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
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
        {
            std::string path = (const char*)payload->Data;
            std::filesystem::path texturePath{ path };
            texturePath = project_globals::g_AssetDirectory / texturePath;
            const std::string& ext = texturePath.extension().string();

            FL_LOG("Payload recieved: {0}, with extension {1}", path, ext);

            if (std::filesystem::exists(texturePath) && std::filesystem::is_regular_file(texturePath) && (ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
                sprite.TextureFilePath = texturePath.string();
            else
                FL_WARN("Bad File given as Texture!");
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
