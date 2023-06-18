#include "MaterialEditorPanel.h"

#include <filesystem>
#include "../Utils.h"

namespace Flameberry {
    void MaterialEditorPanel::OnUIRender()
    {
        bool isMaterialEdited = false;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        ImGui::Begin("Material Editor");
        ImGui::PopStyleVar();

        if (ImGui::BeginTable("MaterialAttributeTable", 2, m_TableFlags))
        {
            ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
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
                    isMaterialEdited = true;
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
                        m_EditingContext = AssetManager::TryGetOrLoadAssetFromFile<Material>(matPath);
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

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Texture Map");

                ImGui::TableNextColumn();

                bool& textureMapEnabled = m_EditingContext->TextureMapEnabled;
                ImGui::Checkbox("##Texture_Map", &textureMapEnabled);

                isMaterialEdited = isMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();

                if (textureMapEnabled)
                {
                    auto& texture = m_EditingContext->TextureMap;
                    if (!texture)
                        texture = VulkanTexture::TryGetOrLoadTexture("Assets/Textures/Checkerboard.png");

                    VulkanTexture& currentTexture = *(texture.get());

                    ImGui::SameLine();
                    ImGui::Image(reinterpret_cast<ImTextureID>(currentTexture.GetDescriptorSet()), ImVec2{ 70, 70 });
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
                        {
                            std::string path = (const char*)payload->Data;
                            std::filesystem::path texturePath{ path };
                            const std::string& ext = texturePath.extension().string();

                            FL_LOG("Payload recieved: {0}, with extension {1}", path, ext);

                            if (std::filesystem::exists(texturePath) && std::filesystem::is_regular_file(texturePath) && (ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
                            {
                                texture = VulkanTexture::TryGetOrLoadTexture(texturePath.string());
                                isMaterialEdited = true;
                            }
                            else
                                FL_WARN("Bad File given as Texture!");
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Normal Map");
                ImGui::TableNextColumn();

                bool& normalMapEnabled = m_EditingContext->NormalMapEnabled;
                ImGui::Checkbox("##Normal_Map", &normalMapEnabled);
                isMaterialEdited = isMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();

                if (normalMapEnabled)
                {
                    auto& texture = m_EditingContext->NormalMap;
                    if (!texture)
                        texture = VulkanTexture::TryGetOrLoadTexture("Assets/Textures/Checkerboard.png");

                    VulkanTexture& currentTexture = *(texture.get());

                    ImGui::SameLine();
                    ImGui::Image(reinterpret_cast<ImTextureID>(currentTexture.GetDescriptorSet()), ImVec2{ 70, 70 });
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
                        {
                            std::string path = (const char*)payload->Data;
                            std::filesystem::path texturePath{ path };
                            const std::string& ext = texturePath.extension().string();

                            FL_LOG("Payload recieved: {0}, with extension {1}", path, ext);

                            if (std::filesystem::exists(texturePath) && std::filesystem::is_regular_file(texturePath) && (ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
                            {
                                texture = VulkanTexture::TryGetOrLoadTexture(texturePath.string());
                                isMaterialEdited = true;
                            }
                            else
                                FL_WARN("Bad File given as Texture!");
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Albedo");
                ImGui::TableNextColumn();
                FL_REMOVE_LABEL(ImGui::ColorEdit3("##Albedo", glm::value_ptr(m_EditingContext->Albedo)));
                isMaterialEdited = isMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Roughness");
                ImGui::TableNextColumn();
                FL_REMOVE_LABEL(ImGui::DragFloat("##Roughness", &m_EditingContext->Roughness, 0.01f, 0.0f, 1.0f));
                isMaterialEdited = isMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Metallic");
                ImGui::TableNextColumn();
                bool metallic = m_EditingContext->Metallic;
                ImGui::Checkbox("##Metallic", &metallic);
                isMaterialEdited = isMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
                m_EditingContext->Metallic = metallic;
            }
            ImGui::EndTable();
        }
        ImGui::End();

        if (isMaterialEdited && !m_EditingContext->IsDerived)
            MaterialSerializer::Serialize(m_EditingContext, m_EditingContext->FilePath.c_str());
    }
}