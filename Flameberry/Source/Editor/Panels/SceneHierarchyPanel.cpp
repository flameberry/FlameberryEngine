#include "SceneHierarchyPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <IconFontCppHeaders/IconsLucide.h>

#include "Core/EditorContext.h"
#include "Core/UI.h"
#include "ECS/Components.h"

namespace Flameberry {

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
		: m_Context(context), m_InspectorPanel(CreateRef<InspectorPanel>(m_Context))
	{
	}

	SceneHierarchyPanel::~SceneHierarchyPanel()
	{
	}

	void SceneHierarchyPanel::OnUIRender()
	{
		{
			UI::ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			UI::ScopedStyleColor windowBg(ImGuiCol_WindowBg, Theme::WindowBg);

			ImGui::Begin("Scene Hierarchy");
		}

		m_IsFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		constexpr float padding = 12.0f;
		const float width = ImGui::GetContentRegionAvail().x - 2.0f * padding;
		ImGui::SetCursorPos(ImVec2(padding, 4 + ImGui::GetCursorPosY()));

		{
			UI::ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 1.0f, m_IsSearchBarFocused);
			UI::ScopedStyleVariable frameRounding(ImGuiStyleVar_FrameRounding, 8);
			UI::ScopedStyleColor borderColor(ImGuiCol_Border, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f }, m_IsSearchBarFocused);
			UI::ScopedStyleColor frameBg(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));

			m_IsSearchBarFocused = UI::InputBox("##SceneHierarchySearchBar", width, &m_SearchInputBuffer, ICON_LC_SEARCH " Search...");
		}

		{
			UI::ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 4));
			UI::ScopedStyleColor childBg(ImGuiCol_ChildBg, Theme::WindowBgDark);

			ImGui::BeginChild("##EntityList", ImVec2(-1, -1), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
		}

		// Entity Hierarchy Table
		{
			UI::ScopedStyleColor tableBorderStrong(ImGuiCol_TableBorderStrong, ImVec4(0.01f, 0.01f, 0.01f, 1.0f));
			UI::ScopedStyleColor tableBorderLight(ImGuiCol_TableBorderLight, ImVec4(0.01f, 0.01f, 0.01f, 1.0f));

			ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingStretchProp
				| ImGuiTableFlags_Resizable
				| ImGuiTableFlags_PadOuterX
				| ImGuiTableFlags_BordersInnerV
				| ImGuiTableFlags_NoBordersInBody
				| ImGuiTableFlags_ScrollY;

			if (ImGui::BeginTable("SceneHierarchyTable", 3, tableFlags))
			{
				const std::string label = fmt::format("Label ({} Entities)", m_Context->GetRegistry()->Size());

				ImGui::TableSetupScrollFreeze(3, 1);
				ImGui::TableSetupColumn(ICON_LC_EYE, ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, ImGui::CalcTextSize(ICON_LC_EYE).x);
				ImGui::TableSetupColumn(label.c_str(), ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_IndentEnable);
				ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable, ImGui::GetWindowWidth() / 4.5f);

				ImGui::TableHeadersRow();
				{
					UI::ScopedStyleVariable cellPadding(ImGuiStyleVar_CellPadding, ImVec2(0, 1));
					UI::ScopedStyleVariable indentSpacing(ImGuiStyleVar_IndentSpacing, 12.0f);

					m_IsSelectedNodeDisplayed = false;
					DisplayEntityTree(m_Context->GetWorldEntity());
				}

				ImGui::EndTable();
			}
		}

		// Deselect all entities when left-clicked on blank space
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
			EditorContext::Get()->SelectedEntity = FEntity::Null;

		// Open popup when right-clicked on blank space
		if (ImGui::BeginPopupContextItem("CreateEntityNodeContextMenu", m_PopupFlags))
		{
			DisplayCreateEntityMenu(m_Context->GetWorldEntity());
			ImGui::EndPopup();
		}

		// FIXME: Dropping an entity node on the empty background of the window must parent it to the world node
		// but if I enable this, then this overtakes the entity node's own drop area, which causes this behaviour:
		// Action: Drag the entity and try to drop it on itself.
		// Result: Entity gets reparented to the world entity.
#if 0
		ImGuiWindow* childWindow = ImGui::GetCurrentWindowRead();
		ImRect windowRect = childWindow->Rect();
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && !ImGui::IsAnyItemHovered())
		{
			if (ImGui::BeginDragDropTargetCustom(windowRect, childWindow->ID))
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_SCENE_HIERARCHY_ENTITY_NODE"))
				{
					const FEntity payloadEntity = *((const FEntity*)payload->Data);
					m_Context->ReparentEntity(payloadEntity, FEntity::Null);
				}
				ImGui::EndDragDropTarget();
			}
		}
#endif

		ImGui::EndChild();
		ImGui::End();

		m_InspectorPanel->OnUIRender();
	}

	void SceneHierarchyPanel::RenameNode(std::string& tag)
	{
		ImGui::SameLine();
		ImGui::SetKeyboardFocusHere();

		UI::ScopedStyleVariable framePadding(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 2.5f });

		ImGui::PushItemWidth(-1.0f);
		if (ImGui::InputText("###Rename", &m_RenameBuffer, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
		{
			tag = m_RenameBuffer;
			m_RenamedEntity = FEntity::Null;
		}
		ImGui::PopItemWidth();

		// Remove the input box when it is defocused/deactivated
		if (ImGui::IsItemDeactivated())
			m_RenamedEntity = FEntity::Null;
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_InspectorPanel->SetContext(m_Context);
	}

	void SceneHierarchyPanel::DisplayEntityTree(FEntity entity)
	{
		// "Name" of the entity
		auto& tag = m_Context->GetRegistry()->GetComponent<TagComponent>(entity).Tag;

		// If the current entity matches the search, then it is to be highlighted
		bool highlight = false;

		// Search function
		if (!m_SearchInputBuffer.empty())
		{
			// TODO: Maybe some optimisation to not search again if the input string is same
			if (Algorithm::KmpSearch(tag.c_str(), m_SearchInputBuffer.c_str(), true) != -1)
				highlight = true;
		}

		// Entity state
		const bool isWorldEntity = m_Context->IsWorldEntity(entity);
		const bool isCollectionEntity = m_Context->GetRegistry()->HasComponent<CollectionComponent>(entity);
		const bool isRenamed = m_RenamedEntity == entity;
		const bool isSelected = EditorContext::Get()->SelectedEntity == entity;

		m_IsSelectedNodeDisplayed = m_IsSelectedNodeDisplayed || isSelected;
		FBY_ASSERT(!(isWorldEntity && isCollectionEntity), "World Entity cannot be a Collection Entity!");

		bool hasChild = false;

		if (auto* relation = m_Context->GetRegistry()->TryGetComponent<RelationshipComponent>(entity))
			hasChild = relation->FirstChild != FEntity::Null;

		const int treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_AllowOverlap
			| (isSelected ? ImGuiTreeNodeFlags_Selected : 0)
			| (hasChild ? 0 : ImGuiTreeNodeFlags_Leaf)
			| ImGuiTreeNodeFlags_SpanFullWidth
			| ImGuiTreeNodeFlags_SpanAllColumns
			| ImGuiTreeNodeFlags_LabelSpanAllColumns;

		bool shouldDeleteEntity = false, shouldDuplicateEntity = false;
		bool isEntityTreeNodeOpen = false;

		ImGui::PushID((const void*)(uint64_t)entity);
		{
			const float greyShade = isSelected ? 75.0f / 255.0f : 126.0f / 255.0f;
			const ImVec4 greyColor(greyShade, greyShade, greyShade, 1.0f);
			const float textColor = isSelected ? 0.0f : 1.0f;

			{
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();

				// TODO: Add actual functionality to this
				const bool visibility = true;

				// Only display icon when entity node is hovered
				const bool isEntityNodeHovered = ImGui::GetHoveredID() == ImGui::GetID((const void*)(uint64_t)entity);
				ImGui::TextColored(isEntityNodeHovered ? ImVec4(textColor, textColor, textColor, 1.0f) : ImVec4(0, 0, 0, 0), visibility ? ICON_LC_EYE : ICON_LC_EYE_OFF);

				// if (ImGui::IsItemClicked())
				// 	visibility = !visibility;
			}

			// Set the current entity tree node expanded until the selected node is visible
			if (!m_IsSelectedNodeDisplayed)
				ImGui::SetNextItemOpen(true, ImGuiCond_Always);

			ImGui::TableNextColumn();

			{
				// Styling of the Entity TreeNode
				UI::ScopedStyleColor headerColor(ImGuiCol_Header, Theme::AccentColor); // Main Accent Color
				UI::ScopedStyleColor headerActiveColor(ImGuiCol_HeaderActive, Theme::AccentColorLight, isSelected);
				UI::ScopedStyleColor headerHovered(ImGuiCol_HeaderHovered, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f }, isSelected);
				UI::ScopedStyleVariable framePadding(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 2.5f });
				UI::ScopedStyleVariable itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
				UI::ScopedStyleColor textC(ImGuiCol_Text, ImVec4{ textColor, textColor, textColor, 1.0f });
				UI::ScopedStyleColor textC2(ImGuiCol_Text, Theme::ErrorColor, highlight);

				// Figure out the entity icon to be displayed
				const char* iconCStr = isWorldEntity ? ICON_LC_MOUNTAIN_SNOW : (isCollectionEntity ? ICON_LC_FOLDER_OPEN : ICON_LC_BOX);

				// Display the actual entity node with it's tag
				isEntityTreeNodeOpen = ImGui::TreeNodeEx((const void*)(uint64_t)entity, treeNodeFlags, "%s %s", iconCStr, isRenamed ? "" : tag.c_str());
			}

			// Select entity if clicked
			// Only select the object if it is clicked and not being dragged and not toggled open
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
				EditorContext::Get()->SelectedEntity = entity;

			// World Entity should not be renamed
			// Check for rename shortcuts being used
			if (!isWorldEntity && isSelected && ImGui::IsWindowFocused())
			{
				ImGuiIO& io = ImGui::GetIO();
				if (!io.KeyMods && ImGui::IsKeyPressed(ImGuiKey_Enter)) // TODO: Shouldn't work with modifier but it does
					m_RenamedEntity = entity;
			}

			// Display Context Menu
			if (ImGui::BeginPopupContextItem("EntityNodeContextMenu", m_PopupFlags))
			{
				DisplayCreateEntityMenu(entity);

				// World Entity should not be renamed, duplicated, deleted
				ImGui::BeginDisabled(isWorldEntity);
				{
					if (ImGui::MenuItem(ICON_LC_TEXT_CURSOR_INPUT "\tRename"))
						m_RenamedEntity = entity;

					if (ImGui::MenuItem(ICON_LC_COPY "\tDuplicate Entity"))
						shouldDuplicateEntity = true;

					if (ImGui::MenuItem(ICON_LC_DELETE "\tDelete Entity"))
						shouldDeleteEntity = true;
				}
				ImGui::EndDisabled();

				ImGui::EndPopup();
			}

			// Drag entities to drop them on other entities
			if (ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload("FBY_SCENE_HIERARCHY_ENTITY_NODE", &entity, sizeof(entity), ImGuiCond_Once);
				ImGui::Text("%s", tag.c_str());
				ImGui::EndDragDropSource();
			}

			// Drop entities onto each other to reparent them
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_SCENE_HIERARCHY_ENTITY_NODE"))
				{
					const FEntity payloadEntity = *((const FEntity*)payload->Data);
					m_Context->ReparentEntity(payloadEntity, entity);
				}
				ImGui::EndDragDropTarget();
			}

			// Rename Entity
			if (isRenamed)
			{
				m_RenameBuffer = tag;
				RenameNode(tag);
			}

			// Type Column
			{
				ImGui::TableNextColumn();
				ImGui::TextColored(greyColor, isWorldEntity ? "Root" : (isCollectionEntity ? "Collection" : "Entity"));
			}
		}
		ImGui::PopID();

		// Display all the children of the entity if the current node is expanded
		if (isEntityTreeNodeOpen)
		{
			if (hasChild)
			{
				FEntity child = m_Context->GetRegistry()->GetComponent<RelationshipComponent>(entity).FirstChild;
				while (child != FEntity::Null)
				{
					auto temp = m_Context->GetRegistry()->GetComponent<RelationshipComponent>(child).NextSibling;
					DisplayEntityTree(child);
					child = temp;
				}
			}
			ImGui::TreePop();
		}

		// Perform the Context Menu Actions in a deferred way...
		// to avoid crashes due to incomplete rendering of their children
		if (shouldDuplicateEntity)
		{
			const auto duplicate = m_Context->DuplicateEntity(entity);
			EditorContext::Get()->SelectedEntity = duplicate;
		}

		if (shouldDeleteEntity)
		{
			m_Context->DestroyEntityTree(entity);
			if (EditorContext::Get()->SelectedEntity == entity)
				EditorContext::Get()->SelectedEntity = FEntity::Null;
		}
	}

	void SceneHierarchyPanel::DisplayCreateEntityMenu(FEntity parent)
	{
		static uint32_t collectionCount = 0;

		if (ImGui::BeginMenu(ICON_LC_PLUS "\tCreate"))
		{
			if (parent == m_Context->GetWorldEntity())
			{
				if (ImGui::MenuItem(ICON_LC_LIBRARY "\tCollection"))
				{
					const auto entity = CreateCollectionEntity(fmt::format("Collection - {}", collectionCount), parent);
					EditorContext::Get()->SelectedEntity = entity;
					collectionCount++;
				}
			}

			ImGui::SeparatorText("3D");

			if (ImGui::MenuItem(ICON_LC_SQUARE "\tEmpty"))
			{
				const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Empty", parent);
				EditorContext::Get()->SelectedEntity = entity;
			}
			if (ImGui::MenuItem(ICON_LC_TEXT "\tText"))
			{
				const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Text", parent);
				m_Context->GetRegistry()->EmplaceComponent<TextComponent>(entity);
				EditorContext::Get()->SelectedEntity = entity;
			}
			if (ImGui::MenuItem(ICON_LC_CUBOID "\tMesh"))
			{
				const auto entity = m_Context->CreateEntityWithTagTransformAndParent("StaticMesh", parent);
				m_Context->GetRegistry()->EmplaceComponent<MeshComponent>(entity);
				EditorContext::Get()->SelectedEntity = entity;
			}
			if (ImGui::MenuItem(ICON_LC_CAMERA "\tCamera"))
			{
				const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Camera", parent);
				m_Context->GetRegistry()->EmplaceComponent<CameraComponent>(entity);
				EditorContext::Get()->SelectedEntity = entity;
			}

			ImGui::SeparatorText("Lighting");

			if (ImGui::BeginMenu("Light"))
			{
				if (ImGui::MenuItem(ICON_LC_SUNRISE "\tSky Light"))
				{
					const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Sky Light", parent);
					m_Context->GetRegistry()->EmplaceComponent<SkyLightComponent>(entity);
					EditorContext::Get()->SelectedEntity = entity;
				}
				if (ImGui::MenuItem(ICON_LC_SUN "\tDirectional Light"))
				{
					const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Directional Light", parent);
					m_Context->GetRegistry()->EmplaceComponent<DirectionalLightComponent>(entity);
					EditorContext::Get()->SelectedEntity = entity;
				}
				if (ImGui::MenuItem(ICON_LC_LIGHTBULB "\tPoint Light"))
				{
					const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Point Light", parent);
					m_Context->GetRegistry()->EmplaceComponent<PointLightComponent>(entity);
					EditorContext::Get()->SelectedEntity = entity;
				}
				if (ImGui::MenuItem(ICON_LC_CONE "\tSpot Light"))
				{
					const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Spot Light", parent);
					m_Context->GetRegistry()->EmplaceComponent<SpotLightComponent>(entity);
					EditorContext::Get()->SelectedEntity = entity;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
	}

	FEntity SceneHierarchyPanel::CreateCollectionEntity(const std::string& name, FEntity parent)
	{
		FEntity entity = m_Context->CreateEntityWithTagAndParent(name, parent);
		m_Context->GetRegistry()->EmplaceComponent<CollectionComponent>(entity);
		return entity;
	}

} // namespace Flameberry
