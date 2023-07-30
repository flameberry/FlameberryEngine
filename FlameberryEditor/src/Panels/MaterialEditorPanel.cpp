#include "MaterialEditorPanel.h"

#include <filesystem>
#include "../UI.h"

namespace Flameberry {
    void MaterialEditorPanel::OnUIRender()
    {
        m_IsMaterialEdited = false;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 0));
        ImGui::Begin("Material Editor");
        ImGui::PopStyleVar();

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
        ImGui::PushStyleColor(ImGuiCol_Border, Theme::FrameBorder);

        if (ImGui::BeginTable("MaterialAttributeTable", 2, s_TableFlags))
        {
            ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
            ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("Name");
            ImGui::TableNextColumn();

            if (m_EditingContext && m_ShouldRename)
            {

                strcpy(m_RenameBuffer, m_EditingContext->Name.c_str());
                ImGui::SetKeyboardFocusHere();

                ImGui::PushItemWidth(-1.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 2.5f });
                if (ImGui::InputText("###RenameMaterial", m_RenameBuffer, 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    m_EditingContext->Name = std::string(m_RenameBuffer);
                    m_ShouldRename = false;
                    m_IsMaterialEdited = true;
                }
                ImGui::PopStyleVar();
                ImGui::PopItemWidth();
            }
            else
            {
                ImGui::Button(m_EditingContext ? m_EditingContext->Name.c_str() : "Null", ImVec2(-1.0f, 0.0f));
                if (m_EditingContext && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
                    m_ShouldRename = true;
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
                        m_EditingContext = AssetManager::TryGetOrLoadAsset<Material>(matPath);
                    else
                        FL_WARN("Bad File given as Material!");
                }
                ImGui::EndDragDropTarget();
            }

            if (m_EditingContext)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("FilePath");
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", m_EditingContext->FilePath.c_str());

                DrawMapControls("Texture Map", m_EditingContext->TextureMapEnabled, m_EditingContext->TextureMap);
                DrawMapControls("Normal Map", m_EditingContext->NormalMapEnabled, m_EditingContext->NormalMap);
                DrawMapControls("Roughness Map", m_EditingContext->RoughnessMapEnabled, m_EditingContext->RoughnessMap);
                DrawMapControls("Ambient Occlusion Map", m_EditingContext->AmbientOcclusionMapEnabled, m_EditingContext->AmbientOcclusionMap);
                DrawMapControls("Metallic Map", m_EditingContext->MetallicMapEnabled, m_EditingContext->MetallicMap);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Albedo");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(-1.0f);
                ImGui::ColorEdit3("##Albedo", glm::value_ptr(m_EditingContext->Albedo));
                m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Roughness");
                ImGui::TableNextColumn();
                ImGui::DragFloat("##Roughness", &m_EditingContext->Roughness, 0.01f, 0.0f, 1.0f);
                m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Metallic");
                ImGui::TableNextColumn();
                float metallic = m_EditingContext->Metallic;
                ImGui::DragFloat("##Metallic", &metallic, 0.005f, 0.0f, 1.0f);
                ImGui::PopItemWidth();
                m_EditingContext->Metallic = metallic;
                m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
            }
            ImGui::EndTable();

        }

        ImGui::PopStyleColor(); // Frame Border Color
        ImGui::PopStyleVar(); // Frame Border Size
        ImGui::End();

        if (m_IsMaterialEdited && !m_EditingContext->FilePath.empty())
            MaterialSerializer::Serialize(m_EditingContext, m_EditingContext->FilePath.c_str());
    }

    void MaterialEditorPanel::DrawMapControls(const char* label, bool& mapEnabledVar, std::shared_ptr<Texture2D>& map)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextWrapped("%s", label);

        ImGui::TableNextColumn();

        auto labelStr = "##" + std::string(label);
        ImGui::Checkbox(labelStr.c_str(), &mapEnabledVar);

        m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();

        if (mapEnabledVar)
        {
            if (!map)
                map = AssetManager::TryGetOrLoadAsset<Texture2D>("Assets/Textures/Checkerboard.png");

            ImGui::SameLine();
            ImGui::Image(reinterpret_cast<ImTextureID>(map->GetDescriptorSet()), ImVec2{ 70, 70 });
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
                {
                    std::filesystem::path path = (const char*)payload->Data;
                    const auto& ext = path.extension();
                    FL_INFO("Payload recieved: {0}, with extension {1}", path, ext);

                    if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path) && (ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
                    {
                        map = AssetManager::TryGetOrLoadAsset<Texture2D>(path.string());
                        m_IsMaterialEdited = true;
                    }
                    else
                        FL_WARN("Bad File given as {0}!", label);
                }
                ImGui::EndDragDropTarget();
            }
        }
    }
}
