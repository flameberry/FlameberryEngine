#include "InspectorPanel.h"

#include <filesystem>
#include "../Utils.h"

namespace Flameberry {
    InspectorPanel::InspectorPanel()
        : m_MaterialSelectorPanel(std::make_shared<MaterialSelectorPanel>()),
        m_MaterialEditorPanel(std::make_shared<MaterialEditorPanel>()),
        m_TripleDotsIcon(Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/icons/triple_dots_icon.png"))
    {
    }

    InspectorPanel::InspectorPanel(const std::shared_ptr<Scene>& context)
        : m_Context(context),
        m_MaterialSelectorPanel(std::make_shared<MaterialSelectorPanel>()),
        m_MaterialEditorPanel(std::make_shared<MaterialEditorPanel>()),
        m_TripleDotsIcon(Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/icons/triple_dots_icon.png"))
    {
    }

    void InspectorPanel::OnUIRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });
        ImGui::Begin("Inspector");
        ImGui::PopStyleVar();

        if (m_SelectionContext != fbentt::null)
        {
            // ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4{});
            DrawComponent<IDComponent>("ID Component", this, [&]()
                {
                    auto& ID = m_Context->m_Registry->get<IDComponent>(m_SelectionContext).ID;
                    if (ImGui::BeginTable("TransformComponentAttributes", 2, m_TableFlags))
                    {
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::Text("ID");
                        ImGui::TableNextColumn();
                        ImGui::Text("%llu", (uint64_t)ID);
                        ImGui::EndTable();
                    }
                }
            );

            DrawComponent<TransformComponent>("Transform Component", this, [&]()
                {
                    auto& transform = m_Context->m_Registry->get<TransformComponent>(m_SelectionContext);

                    if (ImGui::BeginTable("TransformComponentAttributes", 2, m_TableFlags))
                    {
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Translation");
                        ImGui::TableNextColumn();

                        float colWidth = ImGui::GetColumnWidth();
                        Utils::DrawVec3Control("Translation", transform.translation, 0.0f, 0.01f, colWidth);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Rotation");
                        ImGui::TableNextColumn();
                        Utils::DrawVec3Control("Rotation", transform.rotation, 0.0f, 0.01f, colWidth);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Scale");
                        ImGui::TableNextColumn();
                        Utils::DrawVec3Control("Scale", transform.scale, 1.0f, 0.01f, colWidth);
                        ImGui::EndTable();
                    }
                }
            );

            DrawComponent<MeshComponent>("Mesh Component", this, [&]()
                {
                    auto& mesh = m_Context->m_Registry->get<MeshComponent>(m_SelectionContext);

                    if (ImGui::BeginTable("MeshComponentAttributes", 2, m_TableFlags))
                    {
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Mesh");

                        ImGui::TableNextColumn();

                        const auto& staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle);
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
                                    const auto& loadedMesh = AssetManager::TryGetOrLoadAsset<StaticMesh>(modelPath);
                                    mesh.MeshHandle = loadedMesh->Handle;
                                }
                                else
                                    FL_WARN("Bad File given as Model!");
                            }
                            ImGui::EndDragDropTarget();
                        }
                        ImGui::EndTable();
                    }

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                    bool is_open = ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth);
                    ImGui::PopStyleVar();
                    if (is_open)
                    {
                        const auto& staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle);
                        if (staticMesh)
                        {
                            constexpr uint32_t limit = 10;
                            ImGui::SetNextWindowSizeConstraints(ImVec2(-1.0f, 10.0f), ImVec2(-1.0f, ImGui::GetTextLineHeightWithSpacing() * limit));

                            ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
                            ImGui::BeginChild("MaterialList", ImVec2(ImGui::GetContentRegionAvail().x, 0), false, window_flags);
                            if (ImGui::BeginTable("MaterialTable", 2, m_TableFlags))
                            {
                                ImGui::TableSetupColumn("Material_Index", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                                ImGui::TableSetupColumn("Material_Name", ImGuiTableColumnFlags_WidthStretch);

                                uint32_t submeshIndex = 0;
                                for (const auto& submesh : staticMesh->GetSubMeshes())
                                {
                                    ImGui::TableNextRow();
                                    ImGui::TableNextColumn();

                                    ImGui::AlignTextToFramePadding();
                                    ImGui::Text("Material %d", submeshIndex);
                                    ImGui::TableNextColumn();

                                    std::shared_ptr<Material> mat;
                                    if (auto it = mesh.OverridenMaterialTable.find(submeshIndex); it != mesh.OverridenMaterialTable.end())
                                        mat = AssetManager::GetAsset<Material>(it->second);
                                    else
                                        mat = AssetManager::GetAsset<Material>(submesh.MaterialHandle);

                                    float width = ImGui::GetTextLineHeightWithSpacing() + 2.0f * ImGui::GetStyle().FramePadding.y;
                                    ImGui::Button(
                                        mat ? mat->Name.c_str() : "Null",
                                        ImVec2(ImGui::GetContentRegionAvail().x - width, 0.0f)
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
                                                const auto& loadedMat = AssetManager::TryGetOrLoadAsset<Material>(matPath);
                                                mesh.OverridenMaterialTable[submeshIndex] = loadedMat->Handle;
                                            }
                                            else
                                                FL_WARN("Bad File given as Material!");
                                        }
                                        ImGui::EndDragDropTarget();
                                    }

                                    ImGui::SameLine();
                                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().ItemSpacing.x - ImGui::GetStyle().FrameRounding);

                                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
                                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                                    ImGui::Button("O", ImVec2(width, 0.0f));
                                    ImGui::PopStyleColor();
                                    ImGui::PopStyleVar();

                                    if (ImGui::IsItemClicked())
                                    {
                                        m_MaterialSelectorPanel->OpenPanel([mesh, submeshIndex](const std::shared_ptr<Material>& material) mutable
                                            {
                                                mesh.OverridenMaterialTable[submeshIndex] = material->Handle;
                                            }
                                        );
                                    }
                                    submeshIndex++;
                                }
                                ImGui::EndTable();
                            }
                            ImGui::EndChild();
                        }
                    }
                }
            );

            DrawComponent<LightComponent>("Light Component", this, [&]()
                {
                    auto& light = m_Context->m_Registry->get<LightComponent>(m_SelectionContext);

                    if (ImGui::BeginTable("LightComponentAttributes", 2, m_TableFlags))
                    {
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                        ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::Text("Color");
                        ImGui::TableNextColumn();
                        FL_PUSH_WIDTH_MAX(ImGui::ColorEdit3("##Color", glm::value_ptr(light.Color)));

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::Text("Intensity");
                        ImGui::TableNextColumn();
                        FL_PUSH_WIDTH_MAX(ImGui::DragFloat("##Intensity", &light.Intensity, 0.1f));
                        ImGui::EndTable();
                    }
                }
            );

            ImGui::Separator();
            if (Utils::ButtonCenteredOnLine("Add Component"))
                ImGui::OpenPopup("AddComponentPopUp");

            if (ImGui::BeginPopup("AddComponentPopUp"))
            {
                if (ImGui::MenuItem("Transform Component"))
                    m_Context->m_Registry->emplace<TransformComponent>(m_SelectionContext);
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
}
