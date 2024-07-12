#include "InspectorPanel.h"

#include <filesystem>

#include <imgui/misc/cpp/imgui_stdlib.h>
#include <IconFontCppHeaders/IconsLucide.h>

#include "Core/UI.h"
#include "ECS/Components.h"
#include "Renderer/Skymap.h"
#include "glm/gtc/type_ptr.hpp"
#include "imgui.h"

namespace Flameberry {

	InspectorPanel::InspectorPanel()
		: m_MaterialSelectorPanel(CreateRef<MaterialSelectorPanel>()), m_MaterialEditorPanel(CreateRef<MaterialEditorPanel>()), m_SettingsIcon(Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/SettingsIcon.png"))
	{
	}

	InspectorPanel::InspectorPanel(const Ref<Scene>& context)
		: m_Context(context), m_MaterialSelectorPanel(CreateRef<MaterialSelectorPanel>()), m_MaterialEditorPanel(CreateRef<MaterialEditorPanel>()), m_SettingsIcon(Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/SettingsIcon2.png"))
	{
	}

	void InspectorPanel::OnUIRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });
		ImGui::Begin("Inspector");
		ImGui::PopStyleVar();

		if (m_SelectionContext != fbentt::null)
		{
			ImGui::PushStyleColor(ImGuiCol_Border, Theme::FrameBorder);

			auto& tag = m_Context->m_Registry->get<TagComponent>(m_SelectionContext);
			ImFont* bigFont = ImGui::GetIO().Fonts->Fonts[0];
			ImGuiStyle& style = ImGui::GetStyle();

			const auto& windowPadding = ImGui::GetStyle().WindowPadding;
			ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + windowPadding.x, ImGui::GetCursorPos().y + windowPadding.y));

			ImGui::PushFont(bigFont);
			ImGui::Text("%s", tag.Tag.c_str());
			ImGui::PopFont();

			const char* addComponentText = ICON_LC_PLUS " Add";

			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - style.FramePadding.x * 2.0f - ImGui::CalcTextSize(addComponentText).x - style.ItemSpacing.x);

			if (ImGui::Button(addComponentText))
				ImGui::OpenPopup("AddComponentPopUp");

			if (ImGui::BeginPopup("AddComponentPopUp"))
			{
				DrawAddComponentEntry<TransformComponent>(ICON_LC_SCALE_3D "\tTransform Component");
				DrawAddComponentEntry<TextComponent>(ICON_LC_TEXT "\tText Component");
				DrawAddComponentEntry<CameraComponent>(ICON_LC_CAMERA "\tCamera Component");
				DrawAddComponentEntry<MeshComponent>(ICON_LC_CUBOID "\tMesh Component");
				DrawAddComponentEntry<SkyLightComponent>(ICON_LC_SUNRISE "\tSky Light Component");
				DrawAddComponentEntry<DirectionalLightComponent>(ICON_LC_SUN "\tDirectional Light Component");
				DrawAddComponentEntry<PointLightComponent>(ICON_LC_LIGHTBULB "\tPoint Light Component");
				DrawAddComponentEntry<SpotLightComponent>(ICON_LC_CONE "\tSpot Light Component");
				DrawAddComponentEntry<RigidBodyComponent>(ICON_LC_BOXES "\tRigid Body Component");
				DrawAddComponentEntry<BoxColliderComponent>(ICON_LC_BOX "\tBox Collider Component");
				DrawAddComponentEntry<SphereColliderComponent>(ICON_LC_CIRCLE_DASHED "\tSphere Collider Component");
				DrawAddComponentEntry<CapsuleColliderComponent>(ICON_LC_PILL "\tCapsule Collider Component");

				ImGui::EndPopup();
			}

			ImGui::Spacing();

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });
			ImGui::PushStyleColor(ImGuiCol_Border, Theme::WindowBorder);
			ImGui::BeginChild("##InspectorPanelComponentArea", ImVec2(-1, -1), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX);
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();

			ImGui::Spacing();
#if 0
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
                        ImGui::Text("%llu", (UUID::value_type)ID);
                        ImGui::EndTable();
                    }
                }, true // removable = false
            );
#endif

			DrawComponent<TransformComponent>(ICON_LC_SCALE_3D " Transform", [&]() {
				auto& transform = m_Context->m_Registry->get<TransformComponent>(m_SelectionContext);

				if (ImGui::BeginTable("TransformComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
					ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Translation");
					ImGui::TableNextColumn();

					float colWidth = ImGui::GetColumnWidth();
					UI::Vec3Control("Translation", transform.Translation, 0.0f, 0.01f, colWidth);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Rotation");
					ImGui::TableNextColumn();
					UI::Vec3Control("Rotation", transform.Rotation, 0.0f, 0.01f, colWidth);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Scale");
					ImGui::TableNextColumn();
					UI::Vec3Control("Scale", transform.Scale, 1.0f, 0.01f, colWidth);
					ImGui::EndTable();
				} }, false // removable = false
			);

			DrawComponent<TextComponent>(ICON_LC_TEXT " Text", [&]() {
				auto& text = m_Context->m_Registry->get<TextComponent>(m_SelectionContext);

				if (ImGui::BeginTable("TextComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
					ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Text");
					ImGui::TableNextColumn();

					ImGui::PushItemWidth(-1.0f);
					ImGui::InputTextMultiline("##Text", &text.TextString);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Font");
					ImGui::TableNextColumn();

					// Display font preview
					ImGui::Image(text.Font->GetAtlasTexture()->CreateOrGetDescriptorSet(), ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_CONTENT_BROWSER_ITEM"))
						{
							const char* path = (const char*)payload->Data;
							std::filesystem::path fontPath(path);
							const std::string& ext = fontPath.extension().string();

							FBY_INFO("Payload recieved: {}, with extension {}", path, ext);

							if (std::filesystem::exists(fontPath) && std::filesystem::is_regular_file(fontPath) && (ext == ".ttf"))
								text.Font = AssetManager::TryGetOrLoadAsset<Font>(fontPath);
							else
								FBY_WARN("Bad File given as Font!");
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::SameLine();
					ImGui::TextWrapped("%s", text.Font->GetName().c_str());

					// FBY_CONTENT_BROWSER_ITEM

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Color");
					ImGui::TableNextColumn();

					ImGui::ColorEdit3("##TextColor", glm::value_ptr(text.Color));

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Kerning");
					ImGui::TableNextColumn();

					ImGui::DragFloat("##Kerning", &text.Kerning, 0.025f);
					ImGui::TableNextRow();

					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Line Spacing");
					ImGui::TableNextColumn();

					ImGui::DragFloat("##Line_Spacing", &text.LineSpacing, 0.025f);

					ImGui::PopItemWidth();

					ImGui::EndTable();
				} }, true // removable = true
			);

			DrawComponent<SkyLightComponent>(ICON_LC_SUNRISE " Sky Light", [=]() {
				auto& skyLightComp = m_Context->m_Registry->get<SkyLightComponent>(m_SelectionContext);

				if (ImGui::BeginTable("SkyLightComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
					ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Color");
					ImGui::TableNextColumn();

					ImGui::PushItemWidth(-1.0f);
					ImGui::ColorEdit3("##Color", glm::value_ptr(skyLightComp.Color));

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Intensity");
					ImGui::TableNextColumn();
					ImGui::DragFloat("##Intensity", &skyLightComp.Intensity, 0.01f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::PopItemWidth();

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Enable SkyMap");
					ImGui::TableNextColumn();
					ImGui::Checkbox("##Enable_EnvMap", &skyLightComp.EnableSkyMap);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					if (skyLightComp.EnableSkyMap)
					{
						ImGui::AlignTextToFramePadding();
						ImGui::Text("SkyMap");
						ImGui::TableNextColumn();

						auto skyMap = AssetManager::GetAsset<Skymap>(skyLightComp.SkyMap);

						ImGui::Button(skyMap ? skyMap->FilePath.filename().c_str() : "Null", ImVec2(-1.0f, 0.0f));

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_CONTENT_BROWSER_ITEM"))
							{
								const char* path = (const char*)payload->Data;
								std::filesystem::path envPath{ path };
								const std::string& ext = envPath.extension().string();

								FBY_INFO("Payload recieved: {}, with extension {}", path, ext);

								if (std::filesystem::exists(envPath) && std::filesystem::is_regular_file(envPath) && (ext == ".hdr"))
									skyLightComp.SkyMap = AssetManager::TryGetOrLoadAsset<Skymap>(envPath)->Handle;
								else
									FBY_WARN("Bad File given as Environment!");
							}
							ImGui::EndDragDropTarget();
						}
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
					}
					ImGui::EndTable();
				}
			});

			DrawComponent<CameraComponent>(ICON_LC_CAMERA " Camera", [&]() {
				auto& cameraComp = m_Context->m_Registry->get<CameraComponent>(m_SelectionContext);

				if (ImGui::BeginTable("CameraComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
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
					ImGui::Text(currentProjectionTypeIndex == (uint8_t)ProjectionType::Perspective ? "FOV" : "Zoom");

					ImGui::TableNextColumn();
					float FOV = cameraComp.Camera.GetSettings().FOV;
					if (ImGui::DragFloat("##FOV", &FOV, 0.01f, 0.0f, 180.0f))
						cameraComp.Camera.UpdateWithFOVorZoom(FOV);

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
			});

			DrawComponent<MeshComponent>(ICON_LC_CUBOID " Mesh", [&]() {
				auto& mesh = m_Context->m_Registry->get<MeshComponent>(m_SelectionContext);

				if (ImGui::BeginTable("MeshComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
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
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_CONTENT_BROWSER_ITEM"))
						{
							const char* path = (const char*)payload->Data;
							std::filesystem::path modelPath{ path };
							const std::string& ext = modelPath.extension().string();

							FBY_INFO("Payload recieved: {}, with extension {}", path, ext);

							if (std::filesystem::exists(modelPath) && std::filesystem::is_regular_file(modelPath) && (ext == ".obj"))
							{
								const auto& loadedMesh = AssetManager::TryGetOrLoadAsset<StaticMesh>(modelPath);
								mesh.MeshHandle = loadedMesh->Handle;
							}
							else
								FBY_WARN("Bad File given as Model!");
						}
						ImGui::EndDragDropTarget();
					}
					ImGui::EndTable();
				}

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
				bool is_open = ImGui::CollapsingHeader(ICON_LC_DRIBBBLE " Materials", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth);
				ImGui::PopStyleVar();
				if (is_open)
				{
					if (auto staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle))
					{
						const uint32_t limit = glm::min<uint32_t>(staticMesh->GetSubMeshes().size(), 8);
						const float textLineHeightWithSpacing = ImGui::GetTextLineHeightWithSpacing() + 2.0f;
						const float verticalLength = textLineHeightWithSpacing + 2.0f * ImGui::GetStyle().CellPadding.y + 2.0f;
						ImGui::SetNextWindowSizeConstraints(ImVec2(-1.0f, verticalLength), ImVec2(-1.0f, verticalLength * limit));

						ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;

						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
						ImGui::BeginChild("MaterialList", ImVec2(-1.0f, 0.0f), false, window_flags);
						ImGui::PopStyleVar();

						if (ImGui::BeginTable("MaterialTable", 4, s_TableFlags))
						{
							ImGui::TableSetupColumn("Material_Index", ImGuiTableColumnFlags_WidthFixed, 60.0f);
							ImGui::TableSetupColumn("Material_Name", ImGuiTableColumnFlags_WidthStretch);
							ImGui::TableSetupColumn("Material_LoadFromAssetsButton", ImGuiTableColumnFlags_WidthFixed, textLineHeightWithSpacing);
							ImGui::TableSetupColumn("Material_ResetButton", ImGuiTableColumnFlags_WidthFixed, textLineHeightWithSpacing);

							uint32_t submeshIndex = 0;
							for (const auto& submesh : staticMesh->GetSubMeshes())
							{
								ImGui::PushID(submeshIndex);

								ImGui::TableNextRow();
								ImGui::TableNextColumn();

								ImGui::AlignTextToFramePadding();
								ImGui::Text("Item %d", submeshIndex);
								ImGui::TableNextColumn();

								Ref<MaterialAsset> mat;
								if (auto it = mesh.OverridenMaterialTable.find(submeshIndex); it != mesh.OverridenMaterialTable.end())
									mat = AssetManager::GetAsset<MaterialAsset>(it->second);
								else
									mat = AssetManager::GetAsset<MaterialAsset>(submesh.MaterialHandle);

								ImGui::Button(
									mat ? mat->GetName().c_str() : "Null",
									ImVec2(-1.0f, 0.0f));

								if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
									ImGui::SetTooltip("%s", mat ? mat->GetName().c_str() : "Null");

								if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
									m_MaterialEditorPanel->DisplayMaterial(mat);

								if (ImGui::BeginDragDropTarget())
								{
									if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_CONTENT_BROWSER_ITEM"))
									{
										const char* path = (const char*)payload->Data;
										std::filesystem::path matPath{ path };
										const std::string& ext = matPath.extension().string();

										FBY_INFO("Payload recieved: {}, with extension {}", path, ext);

										if (std::filesystem::exists(matPath) && std::filesystem::is_regular_file(matPath) && (ext == ".fbmat"))
										{
											const auto& loadedMat = AssetManager::TryGetOrLoadAsset<MaterialAsset>(matPath);
											mesh.OverridenMaterialTable[submeshIndex] = loadedMat->Handle;
										}
										else
											FBY_WARN("Bad File given as Material!");
									}
									ImGui::EndDragDropTarget();
								}

								ImGui::TableNextColumn();

								ImGui::Button(ICON_LC_FOLDER_SEARCH, ImVec2(0.0f, 0.0f));

								if (ImGui::IsItemClicked())
									UI::OpenSelectionWidget("##MaterialSelectionWidget");

								if (UI::BeginSelectionWidget("##MaterialSelectionWidget", m_SearchInputBuffer2, 256))
								{
									for (const auto& [handle, asset] : AssetManager::GetAssetTable())
									{
										if (asset->GetAssetType() == AssetType::Material)
										{
											Ref<MaterialAsset> m = std::static_pointer_cast<MaterialAsset>(asset);

											if (m_SearchInputBuffer2[0] != '\0')
											{
												const int index = Algorithm::KmpSearch(m->GetName().c_str(), m_SearchInputBuffer2, true);
												if (index == -1)
													continue;
											}

											if (UI::SelectionWidgetElement(m->GetName().c_str(), m->Handle == mat->Handle))
												mesh.OverridenMaterialTable[submeshIndex] = m->Handle;
										}
									}
									UI::EndSelectionWidget();
								}

								ImGui::TableNextColumn();
								if (ImGui::Button(ICON_LC_ROTATE_CCW))
									mesh.OverridenMaterialTable.erase(submeshIndex);

								ImGui::PopID();

								submeshIndex++;
							}
							ImGui::EndTable();
						}
						ImGui::EndChild();
					}
				}
			});

			DrawComponent<DirectionalLightComponent>(ICON_LC_SUN " Directional Light", [&]() {
				auto& light = m_Context->m_Registry->get<DirectionalLightComponent>(m_SelectionContext);

				if (ImGui::BeginTable("DirectionalLightComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
					ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Color");
					ImGui::TableNextColumn();

					ImGui::PushItemWidth(-1.0f);
					ImGui::ColorEdit3("##Color", glm::value_ptr(light.Color));

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Intensity");
					ImGui::TableNextColumn();
					ImGui::DragFloat("##Intensity", &light.Intensity, 0.1f);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Light Size");
					ImGui::TableNextColumn();
					ImGui::DragFloat("##LightSize", &light.LightSize, 0.1f, 0.0f, 200.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

					ImGui::PopItemWidth();

					ImGui::EndTable();
				}
			});

			DrawComponent<PointLightComponent>(ICON_LC_LIGHTBULB " Point Light", [&]() {
				auto& light = m_Context->m_Registry->get<PointLightComponent>(m_SelectionContext);

				if (ImGui::BeginTable("PointLightComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
					ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Color");
					ImGui::TableNextColumn();

					ImGui::PushItemWidth(-1.0f);
					ImGui::ColorEdit3("##Color", glm::value_ptr(light.Color));

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Intensity");
					ImGui::TableNextColumn();
					ImGui::DragFloat("##Intensity", &light.Intensity, 0.1f);
					ImGui::PopItemWidth();

					ImGui::EndTable();
				}
			});

			DrawComponent<SpotLightComponent>(ICON_LC_CONE " Spot Light", [&]() {
				auto& light = m_Context->m_Registry->get<SpotLightComponent>(m_SelectionContext);

				if (ImGui::BeginTable("SpotLightComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
					ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Color");
					ImGui::TableNextColumn();

					ImGui::PushItemWidth(-1.0f);
					ImGui::ColorEdit3("##Color", glm::value_ptr(light.Color));

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Intensity");
					ImGui::TableNextColumn();
					ImGui::DragFloat("##Intensity", &light.Intensity, 0.1f);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("InnerConeAngle");
					ImGui::TableNextColumn();

					ImGui::DragFloat("##InnerConeAngle", &light.InnerConeAngle, 0.1f, 0.0f, light.OuterConeAngle);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("OuterConeAngle");
					ImGui::TableNextColumn();

					ImGui::DragFloat("##OuterConeAngle", &light.OuterConeAngle, 0.1f, light.InnerConeAngle, 90.0f);

					ImGui::PopItemWidth();

					ImGui::EndTable();
				}
			});

			DrawComponent<RigidBodyComponent>(ICON_LC_BOXES " Rigid Body", [&]() {
				auto& rigidBody = m_Context->m_Registry->get<RigidBodyComponent>(m_SelectionContext);

				if (ImGui::BeginTable("RigidBodyComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
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
			});

			DrawComponent<BoxColliderComponent>(ICON_LC_BOX " Box Collider", [&]() {
				auto& boxCollider = m_Context->m_Registry->get<BoxColliderComponent>(m_SelectionContext);

				if (ImGui::BeginTable("BoxColliderComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
					ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Collider Size");
					ImGui::TableNextColumn();

					UI::Vec3Control("BoxColliderSize", boxCollider.Size, 1.0f, 0.01f, ImGui::GetColumnWidth());

					ImGui::EndTable();
				}
			});

			DrawComponent<SphereColliderComponent>(ICON_LC_CIRCLE_DASHED " Sphere Collider", [&]() {
				auto& sphereCollider = m_Context->m_Registry->get<SphereColliderComponent>(m_SelectionContext);

				if (ImGui::BeginTable("SphereColliderComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
					ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Radius");
					ImGui::TableNextColumn();

					FBY_PUSH_WIDTH_MAX(ImGui::DragFloat("##Radius", &sphereCollider.Radius, 0.01f, 0.0f, 0.0f));

					ImGui::EndTable();
				}
			});

			DrawComponent<CapsuleColliderComponent>(ICON_LC_PILL " Capsule Collider", [&]() {
				auto& capsuleCollider = m_Context->m_Registry->get<CapsuleColliderComponent>(m_SelectionContext);

				if (ImGui::BeginTable("CapsuleColliderComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
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
								capsuleCollider.Axis = (AxisType)i;
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
			});
			ImGui::PopStyleColor();
		}
		ImGui::EndChild();
		ImGui::End();

		ImGui::PopStyleVar();

		m_MaterialSelectorPanel->OnUIRender();
		m_MaterialEditorPanel->OnUIRender();
	}

} // namespace Flameberry
