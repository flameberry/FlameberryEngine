#include "SceneHierarchyPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "../project.h"
#include "../Utils.h"

namespace Flameberry {
    SceneHierarchyPanel::SceneHierarchyPanel(Scene* scene)
        : m_ActiveScene(scene),
        m_SelectedEntity(ecs::entity_handle::null),
        m_RenamedEntity(ecs::entity_handle::null),
        m_DefaultTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/Checkerboard.png")
    {
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
                auto entity = m_ActiveScene->m_Registry->create();
                m_ActiveScene->m_Registry->emplace<IDComponent>(entity);
                m_ActiveScene->m_Registry->emplace<TagComponent>(entity).Tag = "Empty";
                m_ActiveScene->m_Registry->emplace<TransformComponent>(entity);
                m_SelectedEntity = entity;
            }
            ImGui::EndPopup();
        }

        m_ActiveScene->m_Registry->each([this](ecs::entity_handle& entity) {
            bool should_delete_entity = false;
            auto& tag = m_ActiveScene->m_Registry->get<TagComponent>(entity).Tag;

            bool is_selected = m_SelectedEntity == entity;
            int treeNodeFlags = (is_selected ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
            if (m_RenamedEntity != entity)
                treeNodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;

            ImGui::PushID((uint32_t)entity);

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
                    m_RenamedEntity = entity;

                if (ImGui::MenuItem("Delete Entity"))
                    should_delete_entity = true;
                ImGui::EndPopup();
            }

            if (m_RenamedEntity == entity)
                RenameNode(tag);

            if (should_delete_entity)
            {
                m_ActiveScene->m_Registry->destroy(entity);
                if (is_selected)
                    m_SelectedEntity = ecs::entity_handle::null;
            }

            if (m_SelectedEntity == entity)
                m_ActiveScene->SetSelectedEntity(&entity);

            ImGui::PopID();
            }
        );

        ImGui::End();

        ImGui::Begin("Inspector");
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth;
        if (m_SelectedEntity != ecs::entity_handle::null)
        {
            if (m_ActiveScene->m_Registry->has<IDComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("IDComponent", flags))
                {
                    auto& ID = m_ActiveScene->m_Registry->get<IDComponent>(m_SelectedEntity).ID;
                    ImGui::Text("ID: %llu", (uint64_t)ID);
                    ImGui::Spacing();
                }
            }

            if (m_ActiveScene->m_Registry->has<TransformComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Transform Component", flags))
                {
                    auto& transform = m_ActiveScene->m_Registry->get<TransformComponent>(m_SelectedEntity);
                    ImGui::Spacing();
                    DrawComponent(transform);
                    ImGui::Spacing();
                    ImGui::Spacing();
                }
            }

            if (m_ActiveScene->m_Registry->has<SpriteRendererComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Sprite Renderer Component", flags))
                {
                    auto& sprite = m_ActiveScene->m_Registry->get<SpriteRendererComponent>(m_SelectedEntity);
                    DrawComponent(sprite);
                    ImGui::Spacing();
                }
            }

            if (m_ActiveScene->m_Registry->has<MeshComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Mesh Component", flags))
                {
                    auto& mesh = m_ActiveScene->m_Registry->get<MeshComponent>(m_SelectedEntity);
                    ImGui::Spacing();
                    DrawComponent(mesh);
                }
            }

            if (m_ActiveScene->m_Registry->has<LightComponent>(m_SelectedEntity))
            {
                if (ImGui::CollapsingHeader("Light Component", flags))
                {
                    auto& light = m_ActiveScene->m_Registry->get<LightComponent>(m_SelectedEntity);
                    ImGui::Spacing();
                    DrawComponent(light);
                }
            }

            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Transform Component"))
                    m_ActiveScene->m_Registry->emplace<TransformComponent>(m_SelectedEntity);
                if (ImGui::MenuItem("Sprite Renderer Component"))
                    m_ActiveScene->m_Registry->emplace<SpriteRendererComponent>(m_SelectedEntity);
                if (ImGui::MenuItem("Mesh Component"))
                    m_ActiveScene->m_Registry->emplace<MeshComponent>(m_SelectedEntity);
                if (ImGui::MenuItem("Light Component"))
                    m_ActiveScene->m_Registry->emplace<LightComponent>(m_SelectedEntity);
                ImGui::EndPopup();
            }
        }
        ImGui::End();

        OnEnvironmentMapPanelRender();
    }

    void SceneHierarchyPanel::RenameNode(std::string& tag)
    {
        std::string buffer(tag);

        ImGui::SameLine();
        ImGui::SetKeyboardFocusHere();
        ImGui::PushItemWidth(-1.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0, 0 });
        if (ImGui::InputText("###Rename", buffer.data(), 100, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            tag = buffer;
            m_RenamedEntity = ecs::entity_handle::null;
        }
        ImGui::PopStyleVar();
    }

    void SceneHierarchyPanel::DrawComponent(TransformComponent& transform)
    {
        Utils::DrawVec3Control("Translation", transform.translation, 0.0f, 0.01f);
        ImGui::Spacing();
        Utils::DrawVec3Control("Rotation", transform.rotation, 0.0f, 0.01f);
        ImGui::Spacing();
        Utils::DrawVec3Control("Scale", transform.scale, 1.0f, 0.01f);
    }

    void SceneHierarchyPanel::DrawComponent(SpriteRendererComponent& sprite)
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
            textureID = m_DefaultTexture.GetTextureID();
        else
            textureID = OpenGLRenderCommand::CreateTexture(sprite.TextureFilePath);
        ImGui::Image(reinterpret_cast<ImTextureID>(textureID), ImVec2{ 50, 50 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
    }

    void SceneHierarchyPanel::DrawComponent(MeshComponent& mesh)
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
            auto [vertices, indices] = OpenGLRenderCommand::LoadModel(modelPathAccepted);
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

        uint32_t plusIconTextureID = OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameEditor/icons/plus_icon.png");
        uint32_t minusIconTextureID = OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"FlameEditor/icons/minus_icon.png");

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

        bool& textureMapEnabled = material.TextureMapEnabled;
        ImGui::Checkbox("Texture Map: ", &textureMapEnabled);

        if (textureMapEnabled)
        {
            auto& texture = material.TextureMap;
            if (!texture)
                texture.reset(new OpenGLTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/Checkerboard.png"));

            OpenGLTexture& currentTexture = *(texture.get());

            ImGui::SameLine();
            ImGui::Image(reinterpret_cast<ImTextureID>(currentTexture.GetTextureID()), ImVec2{ 50, 50 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
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
                        texture.reset(new OpenGLTexture(texturePath.string()));
                    else
                        FL_WARN("Bad File given as Texture!");
                }
                ImGui::EndDragDropTarget();
            }
        }

        ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo));
        ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f);
        ImGui::Checkbox("Metallic", &material.Metallic);
    }

    void SceneHierarchyPanel::DrawComponent(LightComponent& light)
    {
        ImGui::ColorEdit3("Color", glm::value_ptr(light.Color));
        ImGui::DragFloat("Intensity", &light.Intensity, 0.1f);
    }

    void SceneHierarchyPanel::OnEnvironmentMapPanelRender()
    {
        ImGui::Begin("Environment");

        auto& environment = m_ActiveScene->m_SceneData.ActiveEnvironmentMap;
        ImGui::ColorEdit3("Clear Color", glm::value_ptr(environment.ClearColor));
        ImGui::Checkbox("Environment Reflections", &environment.Reflections);

        Utils::DrawVec3Control("Directional", environment.DirLight.Direction, 0.0f, 0.01f);
        ImGui::Spacing();
        ImGui::ColorEdit3("Color", glm::value_ptr(environment.DirLight.Color));
        ImGui::DragFloat("Intensity", &environment.DirLight.Intensity, 0.01f);

        ImGui::End();
    }
}
