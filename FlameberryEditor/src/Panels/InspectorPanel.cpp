#include "InspectorPanel.h"

#include <filesystem>
#include "../Utils.h"

namespace Flameberry {
    InspectorPanel::InspectorPanel()
        : m_MaterialSelectorPanel(std::make_shared<MaterialSelectorPanel>()),
        m_MaterialEditorPanel(std::make_shared<MaterialEditorPanel>())
    {
    }

    InspectorPanel::InspectorPanel(const std::shared_ptr<Scene>& context)
        : m_Context(context),
        m_MaterialSelectorPanel(std::make_shared<MaterialSelectorPanel>()),
        m_MaterialEditorPanel(std::make_shared<MaterialEditorPanel>())
    {
    }

    void InspectorPanel::OnUIRender()
    {
        ImGui::Begin("Inspector");
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth;

        if (m_SelectionContext != ecs::entity_handle::null)
        {
            if (m_Context->m_Registry->has<IDComponent>(m_SelectionContext))
            {
                if (ImGui::CollapsingHeader("IDComponent", flags))
                {
                    auto& ID = m_Context->m_Registry->get<IDComponent>(m_SelectionContext).ID;
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, 80.0f);
                    ImGui::Text("ID");
                    ImGui::NextColumn();
                    ImGui::Text("%llu", (uint64_t)ID);
                    ImGui::EndColumns();
                    ImGui::Spacing();
                }
            }

            if (m_Context->m_Registry->has<TransformComponent>(m_SelectionContext))
            {
                if (ImGui::CollapsingHeader("Transform Component", flags))
                {
                    auto& transform = m_Context->m_Registry->get<TransformComponent>(m_SelectionContext);
                    ImGui::Spacing();
                    DrawComponent(transform);
                    ImGui::Spacing();
                    ImGui::Spacing();
                }
            }

            if (m_Context->m_Registry->has<MeshComponent>(m_SelectionContext))
            {
                if (ImGui::CollapsingHeader("Mesh Component", flags))
                {
                    auto& mesh = m_Context->m_Registry->get<MeshComponent>(m_SelectionContext);
                    ImGui::Spacing();
                    DrawComponent(mesh);
                }
            }

            if (m_Context->m_Registry->has<LightComponent>(m_SelectionContext))
            {
                if (ImGui::CollapsingHeader("Light Component", flags))
                {
                    auto& light = m_Context->m_Registry->get<LightComponent>(m_SelectionContext);
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
                    m_Context->m_Registry->emplace<TransformComponent>(m_SelectionContext);
                if (ImGui::MenuItem("Sprite Renderer Component"))
                    m_Context->m_Registry->emplace<SpriteRendererComponent>(m_SelectionContext);
                if (ImGui::MenuItem("Mesh Component"))
                    m_Context->m_Registry->emplace<MeshComponent>(m_SelectionContext);
                if (ImGui::MenuItem("Light Component"))
                    m_Context->m_Registry->emplace<LightComponent>(m_SelectionContext);
                ImGui::EndPopup();
            }
        }
        ImGui::End();

        m_MaterialSelectorPanel->OnUIRender();
        m_MaterialEditorPanel->OnUIRender();
    }

    void InspectorPanel::DrawComponent(TransformComponent& transform)
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

    void InspectorPanel::DrawComponent(MeshComponent& mesh)
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
                            m_MaterialEditorPanel->SetEditingContext(mat);

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

    void InspectorPanel::DrawComponent(LightComponent& light)
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
}