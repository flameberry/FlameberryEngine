#include "MaterialEditorPanel.h"

#include <filesystem>
#include "UI.h"

namespace Flameberry {
	void MaterialEditorPanel::OnUIRender()
	{
		if (m_Open)
		{
			m_IsMaterialEdited = false;
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 0));
			ImGui::Begin("Material Editor", &m_Open);
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
					strcpy(m_RenameBuffer, m_EditingContext->GetName().c_str());
					ImGui::SetKeyboardFocusHere();

					ImGui::PushItemWidth(-1.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 2.5f });
					if (ImGui::InputText("###RenameMaterial", m_RenameBuffer, 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
					{
						m_EditingContext->SetName(std::string(m_RenameBuffer).c_str());
						m_ShouldRename = false;
						m_IsMaterialEdited = true;
					}
					ImGui::PopStyleVar();
					ImGui::PopItemWidth();
				}
				else
				{
					ImGui::Button(m_EditingContext ? m_EditingContext->GetName().c_str() : "Null", ImVec2(-1.0f, 0.0f));
					if (m_EditingContext && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
						m_ShouldRename = true;
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_CONTENT_BROWSER_ITEM"))
					{
						const char*			  path = (const char*)payload->Data;
						std::filesystem::path matPath{ path };
						const std::string&	  ext = matPath.extension().string();

						FBY_INFO("Payload recieved: {}, with extension {}", path, ext);

						if (std::filesystem::exists(matPath) && std::filesystem::is_regular_file(matPath) && (ext == ".fbmat"))
							m_EditingContext = AssetManager::TryGetOrLoadAsset<MaterialAsset>(matPath);
						else
							FBY_WARN("Bad File given as Material!");
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

					MaterialStructGPURepresentation& uniformDataRef = m_EditingContext->GetMaterialDataRef();

					// This part of the UI causes vulkan validation errors whenever any map is updated
					// Because this leads to updating the DescriptorSet which is to be used by the current frame
					// This didn't happen before with the old `Material` class, which used to update the same descriptor in this part of the code only
					Ref<Texture2D> map;
					map = m_EditingContext->m_AlbedoMap;
					if (DrawMapControls("Texture Map", (bool&)uniformDataRef.UseAlbedoMap, map))
						m_EditingContext->SetAlbedoMap(map);

					map = m_EditingContext->m_NormalMap;
					if (DrawMapControls("Normal Map", (bool&)uniformDataRef.UseNormalMap, map))
						m_EditingContext->SetNormalMap(map);

					map = m_EditingContext->m_RoughnessMap;
					if (DrawMapControls("Roughness Map", (bool&)uniformDataRef.UseRoughnessMap, map))
						m_EditingContext->SetRoughnessMap(map);

					map = m_EditingContext->m_AmbientMap;
					if (DrawMapControls("Ambient Map", (bool&)uniformDataRef.UseAmbientMap, map))
						m_EditingContext->SetAmbientMap(map);

					map = m_EditingContext->m_MetallicMap;
					if (DrawMapControls("Metallic Map", (bool&)uniformDataRef.UseMetallicMap, map))
						m_EditingContext->SetMetallicMap(map);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Albedo");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(-1.0f);
					ImGui::ColorEdit3("##Albedo", glm::value_ptr(uniformDataRef.Albedo));
					m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Roughness");
					ImGui::TableNextColumn();
					ImGui::DragFloat("##Roughness", &uniformDataRef.Roughness, 0.01f, 0.0f, 1.0f);
					m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Metallic");
					ImGui::TableNextColumn();
					ImGui::DragFloat("##Metallic", &uniformDataRef.Metallic, 0.005f, 0.0f, 1.0f);
					ImGui::PopItemWidth();
					m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();
				}
				ImGui::EndTable();
			}

			ImGui::PopStyleColor(); // Frame Border Color
			ImGui::PopStyleVar();	// Frame Border Size
			ImGui::End();

			if (m_IsMaterialEdited && !m_EditingContext->FilePath.empty())
				MaterialAssetSerializer::Serialize(m_EditingContext, m_EditingContext->FilePath.c_str());
		}
	}

	bool MaterialEditorPanel::DrawMapControls(const char* label, bool& mapEnabledVar, Ref<Texture2D>& map)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextWrapped("%s", label);

		ImGui::TableNextColumn();

		auto labelStr = "##" + std::string(label);
		ImGui::Checkbox(labelStr.c_str(), &mapEnabledVar);

		m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();

		bool isMapUpdated = false;
		if (mapEnabledVar)
		{
			if (!map)
				map = AssetManager::TryGetOrLoadAsset<Texture2D>("Content/Textures/Checkerboard.png");

			ImGui::SameLine();
			ImGui::Image(reinterpret_cast<ImTextureID>(map->CreateOrGetDescriptorSet()), ImVec2{ 70, 70 });
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_CONTENT_BROWSER_ITEM"))
				{
					std::filesystem::path path = (const char*)payload->Data;
					const auto&			  ext = path.extension();
					FBY_INFO("Payload recieved: {}, with extension {}", path, ext);

					if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path) && (ext == ".png" || ext == ".jpg" || ext == ".jpeg"))
					{
						map = AssetManager::TryGetOrLoadAsset<Texture2D>(path.string());
						isMapUpdated = true;
						m_IsMaterialEdited = true;
					}
					else
						FBY_WARN("Bad File given as {}!", label);
				}
				ImGui::EndDragDropTarget();
			}
		}
		return isMapUpdated;
	}
} // namespace Flameberry
