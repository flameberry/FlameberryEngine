#include "SceneHierarchyPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "../project.h"
#include "../Utils.h"

SceneHierarchyPanel::SceneHierarchyPanel(Flameberry::Scene* scene, std::vector<Flameberry::Mesh>* meshes)
    : m_Scene(scene), m_Meshes(meshes), m_SelectedEntity(UINT32_MAX, false)
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

    m_Scene->GetRegistry()->each([this](Flameberry::entity_handle& entity) {
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

        if (m_Scene->GetRegistry()->HasComponent<Flameberry::MeshComponent>(m_SelectedEntity))
        {
            ImGui::Spacing();
            auto& sprite = *m_Scene->GetRegistry()->GetComponent<Flameberry::MeshComponent>(m_SelectedEntity);
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
            if (ImGui::MenuItem("Mesh Component"))
                m_Scene->GetRegistry()->AddComponent<Flameberry::MeshComponent>(m_SelectedEntity);
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void SceneHierarchyPanel::DrawComponent(Flameberry::TransformComponent& transform)
{
    Utils::DrawVec3Control("Translation", transform.translation, 0.0f, 0.01f);
    ImGui::Spacing();
    Utils::DrawVec3Control("Rotation", transform.rotation, 0.0f, 0.01f);
    ImGui::Spacing();
    Utils::DrawVec3Control("Scale", transform.scale, 1.0f, 0.01f);
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
            texturePath = project::g_AssetDirectory / texturePath;
            const std::string& ext = texturePath.extension().string();

            FL_LOG("Payload recieved: {0}, with extension {1}", path, ext);

            if (std::filesystem::exists(texturePath) && std::filesystem::is_regular_file(texturePath) && (ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
                sprite.TextureFilePath = texturePath.string();
            else
                FL_WARN("Bad File given as Texture!");
        }
        ImGui::EndDragDropTarget();
    }

    uint32_t textureID;
    if (sprite.TextureFilePath == "")
        textureID = m_DefaultTextureId;
    else
        textureID = Flameberry::OpenGLRenderCommand::CreateTexture(sprite.TextureFilePath);
    ImGui::Image(reinterpret_cast<ImTextureID>(textureID), ImVec2{ 50, 50 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
}

void SceneHierarchyPanel::DrawComponent(Flameberry::MeshComponent& mesh)
{
    std::string modelPathAccepted = "";

    ImGui::Button("Load Mesh");
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
        {
            std::string path = (const char*)payload->Data;
            std::filesystem::path modelPath{ path };
            modelPath = project::g_AssetDirectory / modelPath;
            const std::string& ext = modelPath.extension().string();

            FL_LOG("Payload recieved: {0}, with extension {1}", path, ext);

            if (std::filesystem::exists(modelPath) && std::filesystem::is_regular_file(modelPath) && (ext == ".obj"))
                modelPathAccepted = modelPath.string();
            else
                FL_WARN("Bad File given as Model!");
        }
        ImGui::EndDragDropTarget();
    }

    if (modelPathAccepted != "")
    {
        auto [vertices, indices] = Flameberry::ModelLoader::LoadOBJ(modelPathAccepted);
        m_Meshes->emplace_back(vertices, indices);
        mesh.MeshIndex = m_Meshes->size() - 1;
    }

    if (ImGui::BeginCombo("##combo", std::to_string(mesh.MeshIndex).c_str())) // The second parameter is the label previewed before opening the combo.
    {
        for (int n = 0; n < m_Meshes->size(); n++)
        {
            bool is_selected = (mesh.MeshIndex == n); // You can store your selection however you want, outside or inside your objects
            if (ImGui::Selectable(std::to_string(n).c_str(), is_selected))
                mesh.MeshIndex = n;
            if (is_selected)
                ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
        }
        ImGui::EndCombo();
    }
}