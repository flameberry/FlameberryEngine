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
        ImGui::PushStyleColor(ImGuiCol_Header, { 0.15f, 0.1505f, 0.151f, 1.0f });

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
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, m_LabelWidth);
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
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, m_LabelWidth);
                        ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Translation");
                        ImGui::TableNextColumn();

                        float colWidth = ImGui::GetColumnWidth();
                        Utils::DrawVec3Control("Translation", transform.Translation, 0.0f, 0.01f, colWidth);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Rotation");
                        ImGui::TableNextColumn();
                        Utils::DrawVec3Control("Rotation", transform.Rotation, 0.0f, 0.01f, colWidth);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Scale");
                        ImGui::TableNextColumn();
                        Utils::DrawVec3Control("Scale", transform.Scale, 1.0f, 0.01f, colWidth);
                        ImGui::EndTable();
                    }
                }
            );

            DrawComponent<CameraComponent>("Camera Component", this, [&]()
                {
                    auto& cameraComp = m_Context->m_Registry->get<CameraComponent>(m_SelectionContext);

                    if (ImGui::BeginTable("CameraComponentAttributes", 2, m_TableFlags))
                    {
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, m_LabelWidth);
                        ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Is Primary");

                        ImGui::TableNextColumn();
                        ImGui::Checkbox("##IsPrimary", &cameraComp.IsPrimary);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Projection Type");

                        ImGui::TableNextColumn();

                        const char* projectionTypeStrings[] = { "Orthographic", "Perspective" };
                        uint8_t currentProjectionTypeIndex = (uint8_t)cameraComp.Camera.GetSettings().ProjectionType;

                        ImGui::PushItemWidth(-1.0f);
                        if (ImGui::BeginCombo("##ProjectionType", projectionTypeStrings[currentProjectionTypeIndex]))
                        {
                            for (int i = 0; i < 2; i++)
                            {
                                bool isSelected = (i == (uint8_t)cameraComp.Camera.GetSettings().ProjectionType);
                                if (ImGui::Selectable(projectionTypeStrings[i], &isSelected))
                                {
                                    currentProjectionTypeIndex = i;
                                    cameraComp.Camera.SetProjectionType((ProjectionType)i);
                                }

                                if (isSelected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("FOV");

                        ImGui::TableNextColumn();
                        float FOV = cameraComp.Camera.GetSettings().FOV;
                        if (ImGui::DragFloat("##FOV", &FOV, 0.01f, 0.0f, 180.0f))
                            cameraComp.Camera.UpdateWithFOV(FOV);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Near");

                        ImGui::TableNextColumn();
                        float near = cameraComp.Camera.GetSettings().Near;
                        if (ImGui::DragFloat("##Near", &near, 0.01f, 0.0f, 2000.0f))
                            cameraComp.Camera.UpdateWithNear(near);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Far");

                        ImGui::TableNextColumn();
                        float far = cameraComp.Camera.GetSettings().Far;
                        if (ImGui::DragFloat("##Far", &far, 0.01f, 0.0f, 2000.0f))
                            cameraComp.Camera.UpdateWithFar(far);

                        ImGui::PopItemWidth();
                        ImGui::EndTable();
                    }
                }
            );

            DrawComponent<MeshComponent>("Mesh Component", this, [&]()
                {
                    auto& mesh = m_Context->m_Registry->get<MeshComponent>(m_SelectionContext);

                    if (ImGui::BeginTable("MeshComponentAttributes", 2, m_TableFlags))
                    {
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, m_LabelWidth);
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
                            const uint32_t limit = glm::min<uint32_t>(staticMesh->GetSubMeshes().size(), 8);
                            float verticalLength = ImGui::GetTextLineHeightWithSpacing() + 2.0f * ImGui::GetStyle().CellPadding.y + 2.0f;
                            ImGui::SetNextWindowSizeConstraints(ImVec2(-1.0f, verticalLength), ImVec2(-1.0f, verticalLength * limit));

                            ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;

                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                            ImGui::BeginChild("MaterialList", ImVec2(-1.0f, 0.0f), false, window_flags);
                            ImGui::PopStyleVar();

                            if (ImGui::BeginTable("MaterialTable", 2, m_TableFlags))
                            {
                                ImGui::TableSetupColumn("Material_Index", ImGuiTableColumnFlags_WidthFixed, m_LabelWidth);
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

                                    ImGui::Button(
                                        mat ? mat->Name.c_str() : "Null",
                                        ImVec2(ImGui::GetContentRegionAvail().x - verticalLength, 0.0f)
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
                                    ImGui::Button("O", ImVec2(verticalLength, 0.0f));
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
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, m_LabelWidth);
                        ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::PushItemWidth(-1.0f);
                        ImGui::Text("Color");
                        ImGui::TableNextColumn();
                        ImGui::ColorEdit3("##Color", glm::value_ptr(light.Color));

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::Text("Intensity");
                        ImGui::TableNextColumn();
                        ImGui::DragFloat("##Intensity", &light.Intensity, 0.1f);
                        ImGui::PopItemWidth();

                        ImGui::EndTable();
                    }
                }
            );

            DrawComponent<RigidBodyComponent>("RigidBody Component", this, [&]()
                {
                    auto& rigidBody = m_Context->m_Registry->get<RigidBodyComponent>(m_SelectionContext);

                    if (ImGui::BeginTable("RigidBodyComponentAttributes", 2, m_TableFlags))
                    {
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, m_LabelWidth);
                        ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Type");
                        ImGui::TableNextColumn();

                        const char* rigidBodyTypeStrings[] = { "Static", "Dynamic" };
                        uint8_t currentRigidBodyTypeIndex = (uint8_t)rigidBody.Type;

                        ImGui::PushItemWidth(-1.0f);
                        if (ImGui::BeginCombo("##RigidBodyType", rigidBodyTypeStrings[currentRigidBodyTypeIndex]))
                        {
                            for (int i = 0; i < 2; i++)
                            {
                                bool isSelected = (i == (uint8_t)rigidBody.Type);
                                if (ImGui::Selectable(rigidBodyTypeStrings[i], &isSelected))
                                {
                                    currentRigidBodyTypeIndex = i;
                                    rigidBody.Type = (RigidBodyComponent::RigidBodyType)i;
                                }

                                if (isSelected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Density");
                        ImGui::TableNextColumn();
                        ImGui::DragFloat("##Density", &rigidBody.Density, 0.01f, 0.0f, 1000.0f);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Static Friction");
                        ImGui::TableNextColumn();
                        ImGui::DragFloat("##Static_Friction", &rigidBody.StaticFriction, 0.005, 0.0f, 1.0f);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Dynamic Friction");
                        ImGui::TableNextColumn();
                        ImGui::DragFloat("##Dynamic_Friction", &rigidBody.DynamicFriction, 0.005, 0.0f, 1.0f);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Restitution");
                        ImGui::TableNextColumn();
                        ImGui::DragFloat("##Restitution", &rigidBody.Restitution, 0.005f, 0.0f, 1.0f);
                        ImGui::PopItemWidth();

                        ImGui::EndTable();
                    }
                }
            );

            DrawComponent<BoxColliderComponent>("Box Collider Component", this, [&]()
                {
                    auto& boxCollider = m_Context->m_Registry->get<BoxColliderComponent>(m_SelectionContext);

                    if (ImGui::BeginTable("BoxColliderComponentAttributes", 2, m_TableFlags))
                    {
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, m_LabelWidth);
                        ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Collider Size");
                        ImGui::TableNextColumn();

                        Utils::DrawVec3Control("BoxColliderSize", boxCollider.Size, 1.0f, 0.01f, ImGui::GetColumnWidth());

                        ImGui::EndTable();
                    }
                }
            );

            DrawComponent<SphereColliderComponent>("Sphere Collider Component", this, [&]()
                {
                    auto& sphereCollider = m_Context->m_Registry->get<SphereColliderComponent>(m_SelectionContext);

                    if (ImGui::BeginTable("SphereColliderComponentAttributes", 2, m_TableFlags))
                    {
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, m_LabelWidth);
                        ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Radius");
                        ImGui::TableNextColumn();

                        FL_PUSH_WIDTH_MAX(ImGui::DragFloat("##Radius", &sphereCollider.Radius, 0.01f, 0.0f, 0.0f));

                        ImGui::EndTable();
                    }
                }
            );

            DrawComponent<CapsuleColliderComponent>("Capsule Collider Component", this, [&]()
                {
                    auto& capsuleCollider = m_Context->m_Registry->get<CapsuleColliderComponent>(m_SelectionContext);

                    if (ImGui::BeginTable("CapsuleColliderComponentAttributes", 2, m_TableFlags))
                    {
                        ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, m_LabelWidth);
                        ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Axis");
                        ImGui::TableNextColumn();

                        const char* axisTypeStrings[] = { "X", "Y", "Z" };
                        uint8_t currentAxisType = (uint8_t)capsuleCollider.Axis;

                        ImGui::PushItemWidth(-1.0f);
                        if (ImGui::BeginCombo("##AxisType", axisTypeStrings[currentAxisType]))
                        {
                            for (int i = 0; i < 3; i++)
                            {
                                bool isSelected = (i == (uint8_t)capsuleCollider.Axis);
                                if (ImGui::Selectable(axisTypeStrings[i], &isSelected))
                                {
                                    currentAxisType = i;
                                    capsuleCollider.Axis = (CapsuleColliderComponent::AxisType)i;
                                }

                                if (isSelected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Radius");
                        ImGui::TableNextColumn();

                        ImGui::DragFloat("##Radius", &capsuleCollider.Radius, 0.01f, 0.0f, 0.0f);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("Height");
                        ImGui::TableNextColumn();
                        ImGui::DragFloat("##Height", &capsuleCollider.Height, 0.01f, 0.0f, 0.0f);

                        ImGui::PopItemWidth();

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
                if (ImGui::MenuItem("Camera Component"))
                    m_Context->m_Registry->emplace<CameraComponent>(m_SelectionContext);
                if (ImGui::MenuItem("Mesh Component"))
                    m_Context->m_Registry->emplace<MeshComponent>(m_SelectionContext);
                if (ImGui::MenuItem("Light Component"))
                    m_Context->m_Registry->emplace<LightComponent>(m_SelectionContext);
                if (ImGui::MenuItem("RigidBody Component"))
                    m_Context->m_Registry->emplace<RigidBodyComponent>(m_SelectionContext);
                if (ImGui::MenuItem("Box Colllider Component"))
                    m_Context->m_Registry->emplace<BoxColliderComponent>(m_SelectionContext);
                if (ImGui::MenuItem("Sphere Colllider Component"))
                    m_Context->m_Registry->emplace<SphereColliderComponent>(m_SelectionContext);
                if (ImGui::MenuItem("Capsule Colllider Component"))
                    m_Context->m_Registry->emplace<CapsuleColliderComponent>(m_SelectionContext);
                ImGui::EndPopup();
            }

            ImGui::PopStyleColor();
        }

        ImGui::End();

        m_MaterialSelectorPanel->OnUIRender();
        m_MaterialEditorPanel->OnUIRender();
    }
}
