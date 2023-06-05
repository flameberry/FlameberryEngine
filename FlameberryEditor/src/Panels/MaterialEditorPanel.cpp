#include "MaterialEditorPanel.h"

#include <filesystem>
#include "../Utils.h"

namespace Flameberry {
    void MaterialEditorPanel::OnUIRender()
    {
        if (m_EditingContext)
        {
            bool isMaterialEdited = false;

            ImGui::Begin("Material Editor");
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5, 3 });
            if (ImGui::BeginTable("MaterialAttributeTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoKeepColumnsVisible))
            {
                ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Material");
                ImGui::TableNextColumn();

                ImGui::Button(m_EditingContext->Name.c_str(), ImVec2(-1.0f, 0.0f));
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

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("FilePath");
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", m_EditingContext->FilePath.c_str());

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

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

                ImGui::Text("Albedo");
                ImGui::TableNextColumn();
                FL_REMOVE_LABEL(ImGui::ColorEdit3("##Albedo", glm::value_ptr(m_EditingContext->Albedo)));
                isMaterialEdited = isMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Roughness");
                ImGui::TableNextColumn();
                FL_REMOVE_LABEL(ImGui::DragFloat("##Roughness", &m_EditingContext->Roughness, 0.01f, 0.0f, 1.0f));
                isMaterialEdited = isMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Metallic");
                ImGui::TableNextColumn();
                bool metallic = m_EditingContext->Metallic;
                ImGui::Checkbox("##Metallic", &metallic);
                isMaterialEdited = isMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
                m_EditingContext->Metallic = metallic;
                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
            ImGui::End();

            if (isMaterialEdited && !m_EditingContext->IsDerived)
                MaterialSerializer::Serialize(m_EditingContext, m_EditingContext->FilePath.c_str());
        }
    }
}