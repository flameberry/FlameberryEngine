#include "SceneHierarchyPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "../project.h"
#include "../Utils.h"

namespace Flameberry {
    SceneHierarchyPanel::SceneHierarchyPanel(Flameberry::Scene* scene)
        : m_ActiveScene(scene), m_SelectedEntity(UINT32_MAX, false)
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
                auto& entity = m_ActiveScene->m_Registry->CreateEntity();
                m_ActiveScene->m_Registry->AddComponent<Flameberry::IDComponent>(entity);
                m_ActiveScene->m_Registry->AddComponent<Flameberry::TagComponent>(entity)->Tag = "Empty";
                m_ActiveScene->m_Registry->AddComponent<Flameberry::TransformComponent>(entity);
                m_SelectedEntity = entity;
            }
            ImGui::EndPopup();
        }

        m_ActiveScene->m_Registry->each([this](Flameberry::entity_handle& entity) {
            bool should_delete_entity = false;
        static Flameberry::entity_handle* entity_to_be_renamed = nullptr;

        auto& tag = m_ActiveScene->m_Registry->GetComponent<Flameberry::TagComponent>(entity)->Tag;

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
        {
            m_ActiveScene->m_Registry->DestroyEntity(entity);
            if (is_selected)
                m_SelectedEntity = entity_handle{ UINT_MAX, false };
        }

        if (m_SelectedEntity == entity)
            m_ActiveScene->SetSelectedEntity(&entity);

        ImGui::PopID();
            });

        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 5, 5 });
        ImGui::Begin("Inspector");
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
        if (m_SelectedEntity.is_valid())
        {
            if (m_ActiveScene->m_Registry->HasComponent<Flameberry::IDComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("IDComponent", flags))
                {
                    auto& ID = m_ActiveScene->m_Registry->GetComponent<Flameberry::IDComponent>(m_SelectedEntity)->ID;
                    ImGui::Text("ID: %llu", (uint64_t)ID);
                    ImGui::Spacing();
                }
            }

            if (m_ActiveScene->m_Registry->HasComponent<Flameberry::TransformComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Transform Component", flags))
                {
                    auto& transform = *m_ActiveScene->m_Registry->GetComponent<Flameberry::TransformComponent>(m_SelectedEntity);
                    ImGui::Spacing();
                    DrawComponent(transform);
                    ImGui::Spacing();
                    ImGui::Spacing();
                }
            }

            if (m_ActiveScene->m_Registry->HasComponent<Flameberry::SpriteRendererComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Sprite Renderer Component", flags))
                {
                    auto& sprite = *m_ActiveScene->m_Registry->GetComponent<Flameberry::SpriteRendererComponent>(m_SelectedEntity);
                    DrawComponent(sprite);
                    ImGui::Spacing();
                }
            }

            if (m_ActiveScene->m_Registry->HasComponent<Flameberry::MeshComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Mesh Component", flags))
                {
                    auto& mesh = *m_ActiveScene->m_Registry->GetComponent<Flameberry::MeshComponent>(m_SelectedEntity);
                    ImGui::Spacing();
                    DrawComponent(mesh);
                }
            }

            if (ImGui::BeginPopupContextWindow((const char*)__null, ImGuiMouseButton_Right, false))
            {
                if (ImGui::MenuItem("Transform Component"))
                    m_ActiveScene->m_Registry->AddComponent<Flameberry::TransformComponent>(m_SelectedEntity);
                if (ImGui::MenuItem("Sprite Renderer Component"))
                    m_ActiveScene->m_Registry->AddComponent<Flameberry::SpriteRendererComponent>(m_SelectedEntity);
                if (ImGui::MenuItem("Mesh Component"))
                    m_ActiveScene->m_Registry->AddComponent<Flameberry::MeshComponent>(m_SelectedEntity);
                ImGui::EndPopup();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    std::string SceneHierarchyPanel::RenameNode(const char* name)
    {
        std::string tag(name);
        std::string buffer(name);

        ImGui::SameLine();
        ImGui::SetKeyboardFocusHere();
        ImGui::PushItemWidth(-1.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0, 0 });
        if (ImGui::InputText("###Rename", buffer.data(), 100, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            tag = buffer;
            // entity_to_be_renamed = nullptr;
        }
        ImGui::PopStyleVar();
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

        ImGui::SameLine();

        if (modelPathAccepted != "")
        {
            auto [vertices, indices] = Flameberry::ModelLoader::LoadOBJ(modelPathAccepted);
            m_ActiveScene->m_SceneData.Meshes.emplace_back(vertices, indices);
            mesh.MeshIndex = (uint32_t)m_ActiveScene->m_SceneData.Meshes.size() - 1;
        }

        // Mesh Menu
        if (ImGui::BeginCombo("##combo", m_ActiveScene->m_SceneData.Meshes[mesh.MeshIndex].Name.c_str())) // The second parameter is the label previewed before opening the combo.
        {
            for (int n = 0; n < m_ActiveScene->m_SceneData.Meshes.size(); n++)
            {
                bool is_selected = (mesh.MeshIndex == n); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(m_ActiveScene->m_SceneData.Meshes[n].Name.c_str(), is_selected))
                    mesh.MeshIndex = n;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
            }
            ImGui::EndCombo();
        }
        ImGui::Spacing();

        // Mesh Texture Options
        auto& currentMesh = m_ActiveScene->m_SceneData.Meshes[mesh.MeshIndex];
        bool isMeshTextured = currentMesh.TextureIDs.size() ? 1 : 0;
        uint32_t currentTextureID = isMeshTextured ? currentMesh.TextureIDs[0] : m_DefaultTextureId;
        std::string textureFilePath = "";

        ImGui::Image(reinterpret_cast<ImTextureID>(currentTextureID), ImVec2{ 50, 50 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
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
                    textureFilePath = texturePath.string();
                else
                    FL_WARN("Bad File given as Texture!");
            }
            ImGui::EndDragDropTarget();
        }

        if (textureFilePath != "")
        {
            if (isMeshTextured)
                currentMesh.TextureIDs[0] = Flameberry::OpenGLRenderCommand::CreateTexture(textureFilePath);
            else
                currentMesh.TextureIDs.emplace_back() = Flameberry::OpenGLRenderCommand::CreateTexture(textureFilePath);
        }

        // Material Menu
        ImGui::Text("Material");
        if (ImGui::BeginCombo("##combo1", mesh.MaterialName.c_str())) // The second parameter is the label previewed before opening the combo.
        {
            for (const auto& [materialName, material] : m_ActiveScene->m_SceneData.Materials)
            {
                bool is_selected = (mesh.MaterialName == materialName); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(materialName.c_str(), is_selected))
                    mesh.MaterialName = materialName;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();

        uint32_t plusIconTextureID = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameEditor/icons/plus_icon.png");
        uint32_t minusIconTextureID = Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameEditor/icons/minus_icon.png");

        if (ImGui::ImageButton(reinterpret_cast<void*>(plusIconTextureID), ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight())))
        {
            mesh.MaterialName = "Material_" + std::to_string(m_ActiveScene->m_SceneData.Materials.size());
            m_ActiveScene->m_SceneData.Materials[mesh.MaterialName] = Material();
        }

        ImGui::SameLine();

        if (ImGui::ImageButton(reinterpret_cast<void*>(minusIconTextureID), ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight())))
        {
            m_ActiveScene->m_SceneData.Materials.erase(mesh.MaterialName);
            mesh.MaterialName = "Default";
        }

        // Material Controls
        auto& material = m_ActiveScene->m_SceneData.Materials[mesh.MaterialName];
        ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo));
        ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f);
        ImGui::Checkbox("Metallic", &material.IsMetal);
    }
}
