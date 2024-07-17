#include "InspectorPanel.h"

#include <filesystem>

#include <imgui/misc/cpp/imgui_stdlib.h>
#include <IconFontCppHeaders/IconsLucide.h>

#include "Core/UI.h"
#include "ECS/Components.h"
#include "Renderer/Skymap.h"
#include "Asset/Importers/TextureImporter.h"
#include "Scripting/ScriptEngine.h"

namespace Flameberry {

	InspectorPanel::InspectorPanel()
		: m_MaterialEditorPanel(CreateRef<MaterialEditorPanel>()), m_SettingsIcon(Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/SettingsIcon.png"))
	{
	}

	InspectorPanel::InspectorPanel(const Ref<Scene>& context)
		: m_Context(context), m_MaterialEditorPanel(CreateRef<MaterialEditorPanel>()), m_SettingsIcon(Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/SettingsIcon2.png"))
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
				DrawAddComponentEntry<ScriptComponent>(ICON_LC_SCROLL "\tScript Component");
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
                    if (UI::BeginKeyValueTable("IDComponentAttributes"))
                    {
						UI::TableKeyElement("ID");
                        ImGui::Text("%llu", (UUID::value_type)ID);
                        UI::EndKeyValueTable();
                    }
                }, true // removable = false
            );
#endif

			DrawComponent<TransformComponent>(ICON_LC_SCALE_3D " Transform", [&]() {
				auto& transform = m_Context->m_Registry->get<TransformComponent>(m_SelectionContext);

				if (UI::BeginKeyValueTable("TransformComponentAttributes"))
				{
					UI::TableKeyElement("Translation");
					UI::Vec3Control("Translation", transform.Translation, 0.0f, 0.01f, ImGui::GetColumnWidth());

					UI::TableKeyElement("Rotation");
					UI::Vec3Control("Rotation", transform.Rotation, 0.0f, 0.01f, ImGui::GetColumnWidth());

					UI::TableKeyElement("Scale");
					UI::Vec3Control("Scale", transform.Scale, 1.0f, 0.01f, ImGui::GetColumnWidth());

					UI::EndKeyValueTable();
				} }, false // removable = false
			);

			DrawComponent<TextComponent>(ICON_LC_TEXT " Text", [&]() {
				auto& text = m_Context->m_Registry->get<TextComponent>(m_SelectionContext);

				if (UI::BeginKeyValueTable("TextComponentAttributes"))
				{
					UI::TableKeyElement("Text");

					ImGui::PushItemWidth(-1.0f); // Why does this need to be called after `UI::TableKeyElement("Text");`?
					ImGui::InputTextMultiline("##Text", &text.TextString);

					UI::TableKeyElement("Font");

					Ref<Font> fontAsset = AssetManager::GetAsset<Font>(text.Font);
					Ref<Texture2D> fontPreview = fontAsset ? fontAsset->GetAtlasTexture() : Font::GetDefault()->GetAtlasTexture();

					// Display font preview
					ImGui::Image(fontPreview->CreateOrGetDescriptorSet(), ImVec2(80, 80), ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_CONTENT_BROWSER_ITEM"))
						{
							const char* path = (const char*)payload->Data;
							std::filesystem::path fontPath(path);
							const std::string& ext = fontPath.extension().string();

							FBY_INFO("Payload recieved: {}, with extension {}", path, ext);

							bool shouldImport = Utils::GetAssetTypeFromFileExtension(ext) == AssetType::Font
								&& std::filesystem::exists(fontPath) 
								&& std::filesystem::is_regular_file(fontPath);

							if (shouldImport)
								text.Font = AssetManager::As<EditorAssetManager>()->ImportAsset(fontPath);
							else
								FBY_WARN("Bad File given as Font!");
						}
						ImGui::EndDragDropTarget();
					}

					if (fontAsset)
					{
						ImGui::SameLine();
						ImGui::TextWrapped("%s", fontAsset->GetName().c_str());
					}

					UI::TableKeyElement("Color");
					ImGui::ColorEdit3("##TextColor", glm::value_ptr(text.Color));

					UI::TableKeyElement("Kerning");
					ImGui::DragFloat("##Kerning", &text.Kerning, 0.025f);

					UI::TableKeyElement("LineSpacing");
					ImGui::DragFloat("##Line_Spacing", &text.LineSpacing, 0.025f);

					ImGui::PopItemWidth();

					UI::EndKeyValueTable();
				} }, true // removable = true
			);

			DrawComponent<SkyLightComponent>(ICON_LC_SUNRISE " Sky Light", [=]() {
				auto& skyLightComp = m_Context->m_Registry->get<SkyLightComponent>(m_SelectionContext);

				if (UI::BeginKeyValueTable("SkyLightComponentAttributes"))
				{
					UI::TableKeyElement("Color");

					ImGui::PushItemWidth(-1.0f);
					ImGui::ColorEdit3("##Color", glm::value_ptr(skyLightComp.Color));

					UI::TableKeyElement("Intensity");
					ImGui::DragFloat("##Intensity", &skyLightComp.Intensity, 0.01f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

					ImGui::PopItemWidth();

					UI::TableKeyElement("Enable Skymap");
					ImGui::Checkbox("##Enable_EnvMap", &skyLightComp.EnableSkymap);

					if (skyLightComp.EnableSkymap)
					{
						UI::TableKeyElement("Skymap");

						Ref<Skymap> skymap = AssetManager::GetAsset<Skymap>(skyLightComp.Skymap);

						ImGui::Button(skymap
								? std::filesystem::path(AssetManager::As<EditorAssetManager>()->GetAssetMetadata(skyLightComp.Skymap).FilePath)
									  .filename()
									  .c_str()
								: "Null",
							ImVec2(-1.0f, 0.0f));

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_CONTENT_BROWSER_ITEM"))
							{
								const char* path = (const char*)payload->Data;
								std::filesystem::path envPath{ path };
								const std::string& ext = envPath.extension().string();

								FBY_INFO("Payload recieved: {}, with extension {}", path, ext);

								bool shouldImport = Utils::GetAssetTypeFromFileExtension(ext) == AssetType::Skymap
									&& std::filesystem::exists(envPath)
									&& std::filesystem::is_regular_file(envPath);

								if (shouldImport)
									skyLightComp.Skymap = AssetManager::As<EditorAssetManager>()->ImportAsset(envPath);
								else
									FBY_WARN("Bad File given as Environment!");
							}
							ImGui::EndDragDropTarget();
						}
					}
					UI::EndKeyValueTable();
				}
			});

			DrawComponent<CameraComponent>(ICON_LC_CAMERA " Camera", [&]() {
				auto& cameraComp = m_Context->m_Registry->get<CameraComponent>(m_SelectionContext);

				if (UI::BeginKeyValueTable("CameraComponentAttributes"))
				{
					UI::TableKeyElement("Is Primary");
					ImGui::Checkbox("##IsPrimary", &cameraComp.IsPrimary);

					UI::TableKeyElement("Projection Type");

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

					UI::TableKeyElement(currentProjectionTypeIndex == (uint8_t)ProjectionType::Perspective ? "FOV" : "Zoom");

					float FOV = cameraComp.Camera.GetSettings().FOV;
					if (ImGui::DragFloat("##FOV", &FOV, 0.01f, 0.0f, 180.0f))
						cameraComp.Camera.UpdateWithFOVorZoom(FOV);

					UI::TableKeyElement("Near");

					float near = cameraComp.Camera.GetSettings().Near;
					if (ImGui::DragFloat("##Near", &near, 0.01f, 0.0f, 2000.0f))
						cameraComp.Camera.UpdateWithNear(near);

					UI::TableKeyElement("Far");

					float far = cameraComp.Camera.GetSettings().Far;
					if (ImGui::DragFloat("##Far", &far, 0.01f, 0.0f, 2000.0f))
						cameraComp.Camera.UpdateWithFar(far);

					ImGui::PopItemWidth();

					UI::EndKeyValueTable();
				}
			});

			DrawComponent<MeshComponent>(ICON_LC_CUBOID " Mesh", [&]() {
				auto& mesh = m_Context->m_Registry->get<MeshComponent>(m_SelectionContext);

				if (UI::BeginKeyValueTable("MeshComponentAttributes"))
				{
					UI::TableKeyElement("Mesh");

					Ref<StaticMesh> staticMesh = AssetManager::GetAsset<StaticMesh>(mesh.MeshHandle);
					ImGui::Button(staticMesh ? staticMesh->GetName().c_str() : "Null", ImVec2(-1.0f, 0.0f));

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_CONTENT_BROWSER_ITEM"))
						{
							const char* path = (const char*)payload->Data;
							std::filesystem::path modelPath{ path };
							const std::string& ext = modelPath.extension().string();

							FBY_INFO("Payload recieved: {}, with extension {}", path, ext);

							bool shouldImport = Utils::GetAssetTypeFromFileExtension(ext) == AssetType::StaticMesh
								&& std::filesystem::exists(modelPath)
								&& std::filesystem::is_regular_file(modelPath);

							if (shouldImport)
								mesh.MeshHandle = AssetManager::As<EditorAssetManager>()->ImportAsset(modelPath);
							else
								FBY_WARN("Bad File given as Model!");
						}
						ImGui::EndDragDropTarget();
					}
					UI::EndKeyValueTable();
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

										bool shouldImport = Utils::GetAssetTypeFromFileExtension(ext) == AssetType::Material
											&& std::filesystem::exists(matPath)
											&& std::filesystem::is_regular_file(matPath);

										if (shouldImport)
											mesh.OverridenMaterialTable[submeshIndex] = AssetManager::As<EditorAssetManager>()->ImportAsset(matPath);
										else
											FBY_WARN("Bad File given as Material!");
									}
									ImGui::EndDragDropTarget();
								}

								ImGui::TableNextColumn();

								ImGui::Button(ICON_LC_FOLDER_SEARCH, ImVec2(0.0f, 0.0f));

								if (ImGui::IsItemClicked())
									UI::OpenSelectionWidget("##MaterialSelectionWidget");

								if (UI::BeginSelectionWidget("##MaterialSelectionWidget", &m_SearchInputBuffer2))
								{
									for (const auto& [handle, asset] : AssetManager::As<EditorAssetManager>()->GetLoadedAssets()) // TODO: Revisit
									{
										if (asset->GetAssetType() == AssetType::Material)
										{
											Ref<MaterialAsset> m = std::static_pointer_cast<MaterialAsset>(asset);

											if (m_SearchInputBuffer2[0] != '\0')
											{
												const int index = Algorithm::KmpSearch(m->GetName().c_str(), m_SearchInputBuffer2.c_str(), true);
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

				if (UI::BeginKeyValueTable("DirectionalLightComponentAttributes"))
				{
					UI::TableKeyElement("Color");

					ImGui::PushItemWidth(-1.0f);
					ImGui::ColorEdit3("##Color", glm::value_ptr(light.Color));

					UI::TableKeyElement("Intensity");
					ImGui::DragFloat("##Intensity", &light.Intensity, 0.1f);

					UI::TableKeyElement("Light Size");
					ImGui::DragFloat("##LightSize", &light.LightSize, 0.1f, 0.0f, 200.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

					ImGui::PopItemWidth();
					UI::EndKeyValueTable();
				}
			});

			DrawComponent<PointLightComponent>(ICON_LC_LIGHTBULB " Point Light", [&]() {
				auto& light = m_Context->m_Registry->get<PointLightComponent>(m_SelectionContext);

				if (UI::BeginKeyValueTable("PointLightComponentAttributes"))
				{
					UI::TableKeyElement("Color");

					ImGui::PushItemWidth(-1.0f);
					ImGui::ColorEdit3("##Color", glm::value_ptr(light.Color));

					UI::TableKeyElement("Intensity");
					ImGui::DragFloat("##Intensity", &light.Intensity, 0.1f);

					ImGui::PopItemWidth();
					UI::EndKeyValueTable();
				}
			});

			DrawComponent<SpotLightComponent>(ICON_LC_CONE " Spot Light", [&]() {
				auto& light = m_Context->m_Registry->get<SpotLightComponent>(m_SelectionContext);

				if (UI::BeginKeyValueTable("SpotLightComponentAttributes"))
				{
					UI::TableKeyElement("Color");

					ImGui::PushItemWidth(-1.0f);
					ImGui::ColorEdit3("##Color", glm::value_ptr(light.Color));

					UI::TableKeyElement("Intensity");
					ImGui::DragFloat("##Intensity", &light.Intensity, 0.1f);

					UI::TableKeyElement("InnerConeAngle");
					ImGui::DragFloat("##InnerConeAngle", &light.InnerConeAngle, 0.1f, 0.0f, light.OuterConeAngle);

					UI::TableKeyElement("OuterConeAngle");
					ImGui::DragFloat("##OuterConeAngle", &light.OuterConeAngle, 0.1f, light.InnerConeAngle, 90.0f);

					ImGui::PopItemWidth();
					UI::EndKeyValueTable();
				}
			});

			DrawComponent<ScriptComponent>(ICON_LC_SCROLL " Script", [&]() {
				auto& sc = m_Context->m_Registry->get<ScriptComponent>(m_SelectionContext);

				if (ImGui::BeginTable("ScriptComponentAttributes", 2, s_TableFlags))
				{
					ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, s_LabelWidth);
					ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::AlignTextToFramePadding();
					ImGui::Text("Class Name");
					ImGui::TableNextColumn();

					const auto& actorClasses = ScriptEngine::GetActorClassDictionary();

					ImGui::PushItemWidth(-1.0f);

					ImGui::Button(sc.AssemblyQualifiedClassName.c_str(),
						ImVec2(-1.0f, 0.0f));

					if (ImGui::IsItemClicked())
						UI::OpenSelectionWidget("##ScriptActorClasses");

					if (UI::BeginSelectionWidget("##ScriptActorClasses", &m_SearchInputBuffer1))
					{
						for (const auto& [actorClassName, _] : actorClasses)
						{
							if (!m_SearchInputBuffer1.empty())
							{
								const int index = Algorithm::KmpSearch(actorClassName.c_str(), m_SearchInputBuffer1.c_str(), true);
								if (index == -1)
									continue;
							}

							bool isSelected = (actorClassName == sc.AssemblyQualifiedClassName);

							if (UI::SelectionWidgetElement(actorClassName.c_str(), isSelected))
								sc.AssemblyQualifiedClassName = actorClassName;
						}
						UI::EndSelectionWidget();
					}

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					if (ImGui::CollapsingHeader(
							ICON_LC_RECTANGLE_ELLIPSIS " Fields",
							ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAllColumns))
					{
						if (m_Context->IsRuntimeActive())
						{
							// Display Script Fields
							if (Ref<ManagedActor> managedActor =
									ScriptEngine::GetManagedActor(m_SelectionContext))
							{
								for (const auto& field :
									managedActor->GetManagedClass()->GetScriptFields())
								{
									ImGui::PushID(field.Name.c_str());
									ImGui::TableNextRow();
									ImGui::TableNextColumn();

									ImGui::AlignTextToFramePadding();
									ImGui::Text("%s", field.Name.c_str());

									ImGui::TableNextColumn();

									switch (field.Type)
									{
										case ScriptFieldType::Float:
										{
											auto value = managedActor->GetFieldValue<float>(field);
											if (ImGui::DragFloat("##", &value, 0.1f))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::Double:
										{
											auto value = managedActor->GetFieldValue<double>(field);
											if (ImGui::DragScalar("##", ImGuiDataType_Double, &value,
													0.1f))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::Boolean:
										{
											auto value = managedActor->GetFieldValue<bool>(field);
											if (ImGui::Checkbox("##", &value))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::Char:
										{
											auto value = managedActor->GetFieldValue<int8_t>(field);
											if (ImGui::DragScalar("##", ImGuiDataType_S8, &value, 1))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::Short:
										{
											auto value = managedActor->GetFieldValue<int16_t>(field);
											if (ImGui::DragScalar("##", ImGuiDataType_S16, &value, 1))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::Int:
										{
											auto value = managedActor->GetFieldValue<int32_t>(field);
											if (ImGui::DragScalar("##", ImGuiDataType_S32, &value, 1))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::Long:
										{
											auto value = managedActor->GetFieldValue<int64_t>(field);
											if (ImGui::DragScalar("##", ImGuiDataType_S64, &value, 1))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::Byte:
										{
											auto value = managedActor->GetFieldValue<uint8_t>(field);
											if (ImGui::DragScalar("##", ImGuiDataType_U8, &value, 1))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::UShort:
										{
											auto value = managedActor->GetFieldValue<uint16_t>(field);
											if (ImGui::DragScalar("##", ImGuiDataType_U16, &value, 1))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::UInt:
										{
											auto value = managedActor->GetFieldValue<uint32_t>(field);
											if (ImGui::DragScalar("##", ImGuiDataType_U32, &value, 1))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::ULong:
										{
											auto value = managedActor->GetFieldValue<uint64_t>(field);
											if (ImGui::DragScalar("##", ImGuiDataType_U64, &value, 1))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::Vector2:
										{
											auto value =
												managedActor->GetFieldValue<glm::vec2>(field);
											if (ImGui::DragFloat2("##", glm::value_ptr(value), 0.1f))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::Vector3:
										{
											auto value =
												managedActor->GetFieldValue<glm::vec3>(field);
											if (ImGui::DragFloat3("##", glm::value_ptr(value), 0.1f))
												managedActor->SetFieldValue(field, value);
											break;
										}
										case ScriptFieldType::Vector4:
										{
											auto value =
												managedActor->GetFieldValue<glm::vec4>(field);
											if (ImGui::DragFloat4("##", glm::value_ptr(value), 0.1f))
												managedActor->SetFieldValue(field, value);
											break;
										}
									}
									ImGui::PopID();
								}
							}
						}
						else if (!sc.AssemblyQualifiedClassName.empty())
						{
							uint32_t i = 0;
							// Display Editor Only Script Fields
							for (const auto& field :
								actorClasses.at(sc.AssemblyQualifiedClassName)
									->GetScriptFields())
							{
								ImGui::PushID(field.Name.c_str());
								ImGui::TableNextRow();
								ImGui::TableNextColumn();

								ImGui::AlignTextToFramePadding();
								ImGui::Text("%s", field.Name.c_str());

								ImGui::TableNextColumn();

								auto& localScriptFieldBufferMap =
									ScriptEngine::GetLocalScriptFieldBufferMap();
								ScriptFieldBuffer* scriptFieldBufferPtr = nullptr;
								if (auto scriptFieldBufferMapIterator =
										localScriptFieldBufferMap.find(m_SelectionContext);
									scriptFieldBufferMapIterator != localScriptFieldBufferMap.end())
								{
									if (auto scriptFieldBufferIterator =
											scriptFieldBufferMapIterator->second.find(i);
										scriptFieldBufferIterator != scriptFieldBufferMapIterator->second.end())
										scriptFieldBufferPtr = &scriptFieldBufferIterator->second;
								}

								switch (field.Type)
								{
									case ScriptFieldType::Float:
									{
										auto value = scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<float>()
											: 0.0f;
										if (ImGui::DragFloat("##", &value, 0.1f))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::Double:
									{
										auto value = scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<double>()
											: 0.0f;
										if (ImGui::DragScalar("##", ImGuiDataType_Double, &value,
												0.1f))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::Boolean:
									{
										auto value = scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<bool>()
											: false;
										if (ImGui::Checkbox("##", &value))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::Char:
									{
										auto value = scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<int8_t>()
											: 0;
										if (ImGui::DragScalar("##", ImGuiDataType_S8, &value, 1))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::Short:
									{
										auto value = scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<int16_t>()
											: 0;
										if (ImGui::DragScalar("##", ImGuiDataType_S16, &value, 1))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::Int:
									{
										auto value = scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<int32_t>()
											: 0;
										if (ImGui::DragScalar("##", ImGuiDataType_S32, &value, 1))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::Long:
									{
										auto value = scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<int64_t>()
											: 0;
										if (ImGui::DragScalar("##", ImGuiDataType_S64, &value, 1))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::Byte:
									{
										auto value = scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<uint8_t>()
											: 0;
										if (ImGui::DragScalar("##", ImGuiDataType_U8, &value, 1))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::UShort:
									{
										auto value =
											scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<uint16_t>()
											: 0;
										if (ImGui::DragScalar("##", ImGuiDataType_U16, &value, 1))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::UInt:
									{
										auto value =
											scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<uint32_t>()
											: 0;
										if (ImGui::DragScalar("##", ImGuiDataType_U32, &value, 1))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::ULong:
									{
										auto value =
											scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<uint64_t>()
											: 0;
										if (ImGui::DragScalar("##", ImGuiDataType_U64, &value, 1))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::Vector2:
									{
										auto value =
											scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<glm::vec2>()
											: glm::vec2(0.0f);
										if (ImGui::DragFloat2("##", glm::value_ptr(value), 0.1f))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::Vector3:
									{
										auto value =
											scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<glm::vec3>()
											: glm::vec3(0.0f);
										if (ImGui::DragFloat3("##", glm::value_ptr(value), 0.1f))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
									case ScriptFieldType::Vector4:
									{
										auto value =
											scriptFieldBufferPtr
											? scriptFieldBufferPtr->GetValue<glm::vec4>()
											: glm::vec4(0.0f);
										if (ImGui::DragFloat4("##", glm::value_ptr(value), 0.1f))
											scriptFieldBufferPtr
												? scriptFieldBufferPtr->SetValue(value)
												: localScriptFieldBufferMap[m_SelectionContext][i]
														.SetValue(value);
										break;
									}
								}
								ImGui::PopID();
								i++;
							}
						}
					}
					ImGui::EndTable();
				} }, true // removable = true
			);

			DrawComponent<RigidBodyComponent>(ICON_LC_BOXES " Rigid Body", [&]() {
				auto& rigidBody =
					m_Context->m_Registry->get<RigidBodyComponent>(m_SelectionContext);

				if (UI::BeginKeyValueTable("RigidBodyComponentAttributes"))
				{
					UI::TableKeyElement("Type");

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

					UI::TableKeyElement("Density");
					ImGui::DragFloat("##Density", &rigidBody.Density, 0.01f, 0.0f, 1000.0f);

					UI::TableKeyElement("Static Friction");
					ImGui::DragFloat("##Static_Friction", &rigidBody.StaticFriction, 0.005, 0.0f, 1.0f);

					UI::TableKeyElement("Dynamic Friction");
					ImGui::DragFloat("##Dynamic_Friction", &rigidBody.DynamicFriction, 0.005, 0.0f, 1.0f);

					UI::TableKeyElement("Restitution");
					ImGui::DragFloat("##Restitution", &rigidBody.Restitution, 0.005f, 0.0f, 1.0f);

					ImGui::PopItemWidth();
					UI::EndKeyValueTable();
				}
			});

			DrawComponent<BoxColliderComponent>(ICON_LC_BOX " Box Collider", [&]() {
				auto& boxCollider = m_Context->m_Registry->get<BoxColliderComponent>(m_SelectionContext);

				if (UI::BeginKeyValueTable("BoxColliderComponentAttributes"))
				{
					UI::TableKeyElement("Collider Size");
					UI::Vec3Control("BoxColliderSize", boxCollider.Size, 1.0f, 0.01f, ImGui::GetColumnWidth());

					UI::EndKeyValueTable();
				}
			});

			DrawComponent<SphereColliderComponent>(ICON_LC_CIRCLE_DASHED " Sphere Collider", [&]() {
				auto& sphereCollider = m_Context->m_Registry->get<SphereColliderComponent>(m_SelectionContext);

				if (UI::BeginKeyValueTable("SphereColliderComponentAttributes"))
				{
					UI::TableKeyElement("Radius");
					FBY_PUSH_WIDTH_MAX(ImGui::DragFloat("##Radius", &sphereCollider.Radius, 0.01f, 0.0f, 0.0f));

					UI::EndKeyValueTable();
				}
			});

			DrawComponent<CapsuleColliderComponent>(ICON_LC_PILL " Capsule Collider", [&]() {
				auto& capsuleCollider = m_Context->m_Registry->get<CapsuleColliderComponent>(m_SelectionContext);

				if (UI::BeginKeyValueTable("CapsuleColliderComponentAttributes"))
				{
					UI::TableKeyElement("Axis");

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

					UI::TableKeyElement("Radius");
					ImGui::DragFloat("##Radius", &capsuleCollider.Radius, 0.01f, 0.0f, 0.0f);

					UI::TableKeyElement("Height");
					ImGui::DragFloat("##Height", &capsuleCollider.Height, 0.01f, 0.0f, 0.0f);

					ImGui::PopItemWidth();

					UI::EndKeyValueTable();
				}
			});

			ImGui::PopStyleColor();
		}
		ImGui::EndChild();
		ImGui::End();

		ImGui::PopStyleVar();

		m_MaterialEditorPanel->OnUIRender();
	}

} // namespace Flameberry
