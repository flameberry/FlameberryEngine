#include "MaterialEditorPanel.h"

#include <filesystem>

#include "Core/UI.h"
#include "Asset/EditorAssetManager.h"
#include "Renderer/Renderer.h"

namespace Flameberry {

	MaterialEditorPanel::MaterialEditorPanel()
	{
	}

	void MaterialEditorPanel::OnUIRender()
	{
		if (m_Open)
		{
			m_IsMaterialEdited = false;
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 0));
			ImGui::Begin("Material Editor", &m_Open);
			ImGui::PopStyleVar();

			UI::ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 1);
			UI::ScopedStyleColor borderColor(ImGuiCol_Border, Theme::FrameBorder);

			if (UI::BeginKeyValueTable("MaterialAttributeTable"))
			{
				UI::TableKeyElement("Name");

				if (m_EditingContext && m_ShouldRename)
				{
					strcpy(m_RenameBuffer, m_EditingContext->GetName().c_str());
					ImGui::SetKeyboardFocusHere();

					UI::ScopedStyleVariable framePadding(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 2.5f });

					ImGui::PushItemWidth(-1.0f);
					if (ImGui::InputText("###RenameMaterial", m_RenameBuffer, 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
					{
						m_EditingContext->SetName(std::string(m_RenameBuffer).c_str());
						m_ShouldRename = false;
						m_IsMaterialEdited = true;
					}
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
						const char* path = (const char*)payload->Data;
						std::filesystem::path fpath{ path };
						const std::string& ext = fpath.extension().string();

						FBY_INFO("Payload recieved: {}, with extension {}", path, ext);

						bool shouldImport = Utils::GetAssetTypeFromFileExtension(ext) == AssetType::Material
							&& std::filesystem::exists(fpath)
							&& std::filesystem::is_regular_file(fpath);

						if (shouldImport)
						{
							AssetHandle handle = AssetManager::As<EditorAssetManager>()->ImportAsset(fpath);
							m_EditingContext = AssetManager::GetAsset<MaterialAsset>(handle);
						}
						else
							FBY_WARN("Bad File given as Material!");
					}
					ImGui::EndDragDropTarget();
				}

				if (m_EditingContext)
				{
					const auto& metadata = AssetManager::As<EditorAssetManager>()->GetAssetMetadata(m_EditingContext->Handle);

					if (!metadata.IsMemoryAsset)
					{
						UI::TableKeyElement("FilePath");
						ImGui::TextWrapped("%s", metadata.FilePath.c_str());
					}

					MaterialStructGPURepresentation& uniformDataRef = m_EditingContext->GetMaterialDataRef();

					// This part of the UI causes vulkan validation errors whenever any map is updated
					// Because this leads to updating the DescriptorSet which is to be used by the current frame
					// This didn't happen before with the old `Material` class, which used to update the same descriptor in this part of the code only
					AssetHandle mapHandle;
					mapHandle = m_EditingContext->m_AlbedoMap;
					if (DrawMapControls("Texture Map", (bool&)uniformDataRef.UseAlbedoMap, mapHandle))
						m_EditingContext->SetAlbedoMap(mapHandle);

					mapHandle = m_EditingContext->m_NormalMap;
					if (DrawMapControls("Normal Map", (bool&)uniformDataRef.UseNormalMap, mapHandle))
						m_EditingContext->SetNormalMap(mapHandle);

					mapHandle = m_EditingContext->m_RoughnessMap;
					if (DrawMapControls("Roughness Map", (bool&)uniformDataRef.UseRoughnessMap, mapHandle))
						m_EditingContext->SetRoughnessMap(mapHandle);

					mapHandle = m_EditingContext->m_AmbientMap;
					if (DrawMapControls("Ambient Map", (bool&)uniformDataRef.UseAmbientMap, mapHandle))
						m_EditingContext->SetAmbientMap(mapHandle);

					mapHandle = m_EditingContext->m_MetallicMap;
					if (DrawMapControls("Metallic Map", (bool&)uniformDataRef.UseMetallicMap, mapHandle))
						m_EditingContext->SetMetallicMap(mapHandle);

					ImGui::PushItemWidth(-1.0f);

					UI::TableKeyElement("Albedo");
					ImGui::ColorEdit3("##Albedo", glm::value_ptr(uniformDataRef.Albedo));
					m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();

					UI::TableKeyElement("Roughness");
					ImGui::DragFloat("##Roughness", &uniformDataRef.Roughness, 0.01f, 0.0f, 1.0f);
					m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();

					UI::TableKeyElement("Metallic");
					ImGui::DragFloat("##Metallic", &uniformDataRef.Metallic, 0.005f, 0.0f, 1.0f);
					m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();

					ImGui::PopItemWidth();

					// Serialize the material automatically when changed
					if (m_IsMaterialEdited && !metadata.IsMemoryAsset)
						MaterialAssetSerializer::Serialize(m_EditingContext, metadata.FilePath);
				}
				UI::EndKeyValueTable();
			}
			ImGui::End();
		}
	}

	bool MaterialEditorPanel::DrawMapControls(const char* label, bool& mapEnabledVar, AssetHandle& mapHandle)
	{
		UI::TableKeyElement(label);

		auto labelStr = "##" + std::string(label);
		ImGui::Checkbox(labelStr.c_str(), &mapEnabledVar);

		m_IsMaterialEdited = m_IsMaterialEdited || ImGui::IsItemDeactivatedAfterEdit();

		bool isMapUpdated = false;
		if (mapEnabledVar)
		{
			Ref<Texture2D> previewMap = AssetManager::IsAssetHandleValid(mapHandle)
				? AssetManager::GetAsset<Texture2D>(mapHandle)
				: Renderer::GetCheckerboardTexture();

			ImGui::SameLine();
			ImGui::Image(reinterpret_cast<ImTextureID>(previewMap->CreateOrGetDescriptorSet()), ImVec2{ 70, 70 });

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_CONTENT_BROWSER_ITEM"))
				{
					std::filesystem::path path = (const char*)payload->Data;
					const auto& ext = path.extension();
					FBY_INFO("Payload recieved: {}, with extension {}", path, ext);

					bool shouldImport = Utils::GetAssetTypeFromFileExtension(ext) == AssetType::Texture2D
						&& std::filesystem::exists(path)
						&& std::filesystem::is_regular_file(path);

					if (shouldImport)
					{
						mapHandle = AssetManager::As<EditorAssetManager>()->ImportAsset(path);
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
