#include "SceneHierarchyPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "../Utils.h"

namespace Flameberry {
    SceneHierarchyPanel::SceneHierarchyPanel(Scene* scene)
        : m_ActiveScene(scene),
        m_SelectedEntity(ecs::entity_handle::null),
        m_RenamedEntity(ecs::entity_handle::null),
        m_VkTextureSampler(VulkanTexture::GetDefaultSampler()),
        m_PlusIconTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/plus_icon.png", m_VkTextureSampler),
        m_MinusIconTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/minus_icon.png", m_VkTextureSampler),
        m_MaterialSelectorPanel(std::make_shared<MaterialSelectorPanel>()),
        m_MaterialEditorPanel(std::make_shared<MaterialEditorPanel>())
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

                // bool hasRelationShipComponent = m_ActiveScene->m_Registry->has<RelationshipComponent>(entity);
                // if (hasRelationShipComponent && m_ActiveScene->m_Registry->get<RelationshipComponent>(entity).Parent != ecs::entity_handle::null)
                //     return;

                bool is_renamed = m_RenamedEntity == entity;
                bool is_selected = m_SelectedEntity == entity;
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

        m_MaterialSelectorPanel->OnUIRender();
        m_MaterialEditorPanel->OnUIRender();
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

            float colWidth = ImGui::GetColumnWidth();
            Utils::DrawVec3Control("Translation", transform.translation, 0.0f, 0.01f, colWidth);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Rotation");
            ImGui::TableNextColumn();
            Utils::DrawVec3Control("Rotation", transform.rotation, 0.0f, 0.01f, colWidth);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Scale");
            ImGui::TableNextColumn();
            Utils::DrawVec3Control("Scale", transform.scale, 1.0f, 0.01f, colWidth);
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

            ImGui::Text("Mesh");

            ImGui::TableNextColumn();

            const auto& staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshUUID);
            ImGui::Button(staticMesh ? staticMesh->GetName().c_str() : "Null", ImVec2(-1.0f, 0.0f));

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
                {
                    const char* path = (const char*)payload->Data;
                    std::filesystem::path modelPath{ path };
                    const std::string& ext = modelPath.extension().string();

                    FL_INFO("Payload recieved: {0}, with extension {1}", path, ext);

                    if (std::filesystem::exists(modelPath) && std::filesystem::is_regular_file(modelPath) && (ext == ".obj"))
                    {
                        const auto& loadedMesh = AssetManager::TryGetOrLoadAssetFromFile<StaticMesh>(modelPath);
                        mesh.MeshUUID = loadedMesh->GetUUID();
                    }
                    else
                        FL_WARN("Bad File given as Model!");
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::EndTable();
        }

        if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth))
        {
            const auto& staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshUUID);
            if (staticMesh)
            {
                constexpr uint32_t limit = 10;
                ImGui::SetNextWindowSizeConstraints(ImVec2(-1.0f, 10.0f), ImVec2(-1.0f, ImGui::GetTextLineHeightWithSpacing() * limit));

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
                ImGui::BeginChild("MaterialList", ImVec2(ImGui::GetContentRegionAvail().x, 0), false, window_flags);
                if (ImGui::BeginTable("MaterialTable", 2, m_TableFlags))
                {
                    ImGui::TableSetupColumn("Material_Index");
                    ImGui::TableSetupColumn("Material_Name", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    int i = 0;
                    for (const auto& submesh : staticMesh->GetSubMeshes())
                    {
                        ImGui::Text("Material %d", i);
                        ImGui::TableNextColumn();

                        auto mat = AssetManager::GetAsset<Material>(submesh.MaterialUUID);

                        ImGui::Button(
                            AssetManager::IsAssetHandleValid(submesh.MaterialUUID) ?
                            mat->Name.c_str() : "Null",
                            ImVec2(
                                ImGui::GetContentRegionAvail().x - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ItemSpacing.x - ImGui::GetStyle().WindowPadding.x,
                                0.0f
                            )
                        );

                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                            m_MaterialEditorPanel->SetCurrentMaterial(mat);

                        ImGui::SameLine();
                        ImGui::Button("O", ImVec2(-1.0f, 0.0f));
                        if (ImGui::IsItemClicked())
                        {
                            m_MaterialSelectorPanel->OpenPanel([staticMesh, i](const std::shared_ptr<Material>& material)
                                {
                                    staticMesh->SetMaterialToSubMesh(i, material->GetUUID());
                                }
                            );
                        }

                        if (ImGui::BeginDragDropTarget())
                        {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
                            {
                                const char* path = (const char*)payload->Data;
                                std::filesystem::path matPath{ path };
                                const std::string& ext = matPath.extension().string();

                                FL_INFO("Payload recieved: {0}, with extension {1}", path, ext);

                                if (std::filesystem::exists(matPath) && std::filesystem::is_regular_file(matPath) && (ext == ".fbmat"))
                                {
                                    const auto& loadedMat = AssetManager::TryGetOrLoadAssetFromFile<Material>(matPath);
                                    staticMesh->SetMaterialToSubMesh(i, loadedMat->GetUUID());
                                }
                                else
                                    FL_WARN("Bad File given as Material!");
                            }
                            ImGui::EndDragDropTarget();
                        }

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        i++;
                    }
                    ImGui::EndTable();
                }
                ImGui::EndChild();
            }
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
            float colWidth = ImGui::GetColumnWidth();
            Utils::DrawVec3Control("##Directional_Light", environment.DirLight.Direction, 0.0f, 0.01f, colWidth);

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
