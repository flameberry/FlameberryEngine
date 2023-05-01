#include "SceneHierarchyPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "../project.h"
#include "../Utils.h"

#define FL_REMOVE_LABEL(imgui_widget) { ImGui::PushItemWidth(-1); imgui_widget; ImGui::PopItemWidth(); }

namespace Flameberry {
    SceneHierarchyPanel::SceneHierarchyPanel(Scene* scene)
        : m_ActiveScene(scene),
        m_SelectedEntity(ecs::entity_handle::null),
        m_RenamedEntity(ecs::entity_handle::null),
        m_VkTextureSampler(VulkanRenderCommand::CreateDefaultSampler()),
        m_PlusIconTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/plus_icon.png", m_VkTextureSampler),
        m_MinusIconTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/minus_icon.png", m_VkTextureSampler),
        m_PlusIconTextureID(ImGui_ImplVulkan_AddTexture(m_VkTextureSampler, m_PlusIconTexture.GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
        m_MinusIconTextureID(ImGui_ImplVulkan_AddTexture(m_VkTextureSampler, m_MinusIconTexture.GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
    {
    }

    SceneHierarchyPanel::~SceneHierarchyPanel()
    {
        vkDestroySampler(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), m_VkTextureSampler, nullptr);
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
                    auto entity = m_ActiveScene->m_Registry->create();
                    m_ActiveScene->m_Registry->emplace<IDComponent>(entity);
                    m_ActiveScene->m_Registry->emplace<TagComponent>(entity).Tag = "Empty";
                    m_ActiveScene->m_Registry->emplace<TransformComponent>(entity);
                    m_SelectedEntity = entity;
                }
                if (ImGui::MenuItem("Mesh"))
                {
                    auto entity = m_ActiveScene->m_Registry->create();
                    m_ActiveScene->m_Registry->emplace<IDComponent>(entity);
                    m_ActiveScene->m_Registry->emplace<TagComponent>(entity).Tag = "Mesh";
                    m_ActiveScene->m_Registry->emplace<TransformComponent>(entity);
                    m_ActiveScene->m_Registry->emplace<MeshComponent>(entity);
                    m_SelectedEntity = entity;
                }
                if (ImGui::BeginMenu("Light"))
                {
                    if (ImGui::MenuItem("Point Light"))
                    {
                        auto entity = m_ActiveScene->m_Registry->create();
                        m_ActiveScene->m_Registry->emplace<IDComponent>(entity);
                        m_ActiveScene->m_Registry->emplace<TagComponent>(entity).Tag = "Point Light";
                        m_ActiveScene->m_Registry->emplace<TransformComponent>(entity);
                        m_ActiveScene->m_Registry->emplace<LightComponent>(entity);
                        m_SelectedEntity = entity;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        m_ActiveScene->m_Registry->each([this](ecs::entity_handle& entity)
            {
                auto& tag = m_ActiveScene->m_Registry->get<TagComponent>(entity).Tag;

                bool is_renamed = m_RenamedEntity == entity;
                bool is_selected = m_SelectedEntity == entity;
                bool should_delete_entity = false;
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

                if (ImGui::TreeNodeEx(is_renamed ? "" : tag.c_str(), treeNodeFlags))
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

                if (is_renamed)
                    RenameNode(tag);

                if (should_delete_entity)
                {
                    m_ActiveScene->m_Registry->destroy(entity);
                    if (is_selected)
                        m_SelectedEntity = ecs::entity_handle::null;
                }

                if (is_selected)
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
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, 80.0f);
                    ImGui::Text("ID");
                    ImGui::NextColumn();
                    ImGui::Text("%llu", (uint64_t)ID);
                    ImGui::EndColumns();
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

            ImGui::Separator();
            if (Utils::ButtonCenteredOnLine("Add Component"))
                ImGui::OpenPopup("AddComponentPopUp");

            if (ImGui::BeginPopup("AddComponentPopUp"))
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
        char* buffer = new char[256];
        memcpy(buffer, tag.c_str(), tag.size());

        ImGui::SameLine();
        ImGui::SetKeyboardFocusHere();
        ImGui::PushItemWidth(-1.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0, 0 });
        if (ImGui::InputText("###Rename", buffer, 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            tag = std::string(buffer);
            m_RenamedEntity = ecs::entity_handle::null;
        }
        ImGui::PopStyleVar();
    }

    void SceneHierarchyPanel::DrawComponent(TransformComponent& transform)
    {
        if (ImGui::BeginTable("TransformComponentAttributes", 2, ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Translation");
            ImGui::TableNextColumn();
            Utils::DrawVec3Control("Translation", transform.translation, 0.0f, 0.01f);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Rotation");
            ImGui::TableNextColumn();
            Utils::DrawVec3Control("Rotation", transform.rotation, 0.0f, 0.01f);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Scale");
            ImGui::TableNextColumn();
            Utils::DrawVec3Control("Scale", transform.scale, 1.0f, 0.01f);
            ImGui::EndTable();
        }
    }

    void SceneHierarchyPanel::DrawComponent(MeshComponent& mesh)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5, 3 });
        if (ImGui::BeginTable("MeshComponentAttributes", 2, m_TableFlags))
        {
            ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Button("Load Mesh");

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
                {
                    std::string path = (const char*)payload->Data;
                    std::filesystem::path modelPath{ path };
                    modelPath = project::g_AssetDirectory / modelPath;
                    const std::string& ext = modelPath.extension().string();

                    FL_INFO("Payload recieved: {0}, with extension {1}", path, ext);

                    if (std::filesystem::exists(modelPath) && std::filesystem::is_regular_file(modelPath) && (ext == ".obj"))
                    {
                        m_ActiveScene->m_SceneData.Meshes.emplace_back(std::make_shared<VulkanMesh>(modelPath.string()));
                        mesh.MeshIndex = (uint32_t)m_ActiveScene->m_SceneData.Meshes.size() - 1;
                    }
                    else
                        FL_WARN("Bad File given as Model!");
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::TableNextColumn();

            // Mesh Menu
            const char* selectedMeshName = m_ActiveScene->m_SceneData.Meshes.size() ? (m_ActiveScene->m_SceneData.Meshes[mesh.MeshIndex]->GetName().c_str()) : "";
            if (ImGui::BeginCombo("##combo", selectedMeshName)) // The second parameter is the label previewed before opening the combo.
            {
                for (int n = 0; n < m_ActiveScene->m_SceneData.Meshes.size(); n++)
                {
                    bool is_selected = (mesh.MeshIndex == n); // You can store your selection however you want, outside or inside your objects
                    if (ImGui::Selectable(m_ActiveScene->m_SceneData.Meshes[n]->GetName().c_str(), is_selected))
                        mesh.MeshIndex = n;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                }
                ImGui::EndCombo();
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            // Material Menu
            ImGui::Text("Material");

            ImGui::TableNextColumn();

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

            if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_PlusIconTextureID), ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight())))
            {
                mesh.MaterialName = "Material_" + std::to_string(m_ActiveScene->m_SceneData.Materials.size());
                m_ActiveScene->m_SceneData.Materials[mesh.MaterialName] = Material();
            }

            ImGui::SameLine();

            if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_MinusIconTextureID), ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight())))
            {
                m_ActiveScene->m_SceneData.Materials.erase(mesh.MaterialName);
                mesh.MaterialName = "Default";
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            // Material Controls
            auto& material = m_ActiveScene->m_SceneData.Materials[mesh.MaterialName];

            // ImGui::Text("Texture Map");

            // ImGui::TableNextColumn();

            // bool& textureMapEnabled = material.TextureMapEnabled;
            // ImGui::Checkbox("##Texture_Map", &textureMapEnabled);

            // if (textureMapEnabled)
            // {
            //     auto& texture = material.TextureMap;
            //     if (!texture)
            //         texture.reset(new OpenGLTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/Checkerboard.png"));

            //     OpenGLTexture& currentTexture = *(texture.get());

            //     ImGui::SameLine();
            //     ImGui::Image(reinterpret_cast<ImTextureID>(currentTexture.GetTextureID()), ImVec2{ 50, 50 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
            //     if (ImGui::BeginDragDropTarget())
            //     {
            //         if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
            //         {
            //             std::string path = (const char*)payload->Data;
            //             std::filesystem::path texturePath{ path };
            //             texturePath = project::g_AssetDirectory / texturePath;
            //             const std::string& ext = texturePath.extension().string();

            //             FL_LOG("Payload recieved: {0}, with extension {1}", path, ext);

            //             if (std::filesystem::exists(texturePath) && std::filesystem::is_regular_file(texturePath) && (ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
            //                 texture.reset(new OpenGLTexture(texturePath.string()));
            //             else
            //                 FL_WARN("Bad File given as Texture!");
            //         }
            //         ImGui::EndDragDropTarget();
            //     }
            // }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Albedo");
            ImGui::TableNextColumn();
            FL_REMOVE_LABEL(ImGui::ColorEdit3("##Albedo", glm::value_ptr(material.Albedo)));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Roughness");
            ImGui::TableNextColumn();
            FL_REMOVE_LABEL(ImGui::DragFloat("##Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Metallic");
            ImGui::TableNextColumn();
            bool metallic = material.Metallic == 1.0f;
            ImGui::Checkbox("##Metallic", &metallic);
            material.Metallic = metallic ? 1.0f : 0.0f;
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    }

    void SceneHierarchyPanel::DrawComponent(LightComponent& light)
    {
        if (ImGui::BeginTable("LightComponentAttributes", 2, m_TableFlags))
        {
            ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Color");
            ImGui::TableNextColumn();
            FL_REMOVE_LABEL(ImGui::ColorEdit3("##Color", glm::value_ptr(light.Color)));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Intensity");
            ImGui::TableNextColumn();
            FL_REMOVE_LABEL(ImGui::DragFloat("##Intensity", &light.Intensity, 0.1f));
            ImGui::EndTable();
        }
    }

    void SceneHierarchyPanel::OnEnvironmentMapPanelRender()
    {
        ImGui::Begin("Environment");

        if (ImGui::BeginTable("Environment_Attributes", 2, m_TableFlags))
        {
            ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            auto& environment = m_ActiveScene->m_SceneData.ActiveEnvironmentMap;

            ImGui::Text("Clear Color");
            ImGui::TableNextColumn();

            FL_REMOVE_LABEL(ImGui::ColorEdit3("##Clear_Color", glm::value_ptr(environment.ClearColor)));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Enable Skybox");
            ImGui::TableNextColumn();
            ImGui::Checkbox("##Enable_Skybox", &environment.EnableSkybox);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            if (environment.EnableSkybox)
            {
                ImGui::Text("Env Reflections");
                ImGui::TableNextColumn();
                ImGui::Checkbox("##Environment_Reflections", &environment.Reflections);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

            }
            else
                environment.Reflections = false;

            ImGui::Text("Directional");
            ImGui::TableNextColumn();
            Utils::DrawVec3Control("##Directional_Light", environment.DirLight.Direction, 0.0f, 0.01f);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Color");
            ImGui::TableNextColumn();

            FL_REMOVE_LABEL(ImGui::ColorEdit3("##Color", glm::value_ptr(environment.DirLight.Color)));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Intensity");
            ImGui::TableNextColumn();
            FL_REMOVE_LABEL(ImGui::DragFloat("##Intensity", &environment.DirLight.Intensity, 0.01f));
            ImGui::EndTable();
        }
        ImGui::End();
    }
}
