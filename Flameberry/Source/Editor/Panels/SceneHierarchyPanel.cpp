#include "SceneHierarchyPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <IconFontCppHeaders/IconsLucide.h>

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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, Theme::WindowBgGrey);
		ImGui::Begin("Scene Hierarchy");
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		m_IsFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		const float padding = 12.0f;
		const float width = ImGui::GetContentRegionAvail().x - 2.0f * padding;
		ImGui::SetCursorPos(ImVec2(padding, 4 + ImGui::GetCursorPosY()));

		{
			UI::ScopedStyleVariable frameBorderSize(ImGuiStyleVar_FrameBorderSize, 1.0f, m_IsSearchBarFocused);
			UI::ScopedStyleVariable frameRounding(ImGuiStyleVar_FrameRounding, 8);
			UI::ScopedStyleColor borderColor(ImGuiCol_Border, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f }, m_IsSearchBarFocused);
			UI::ScopedStyleColor frameBg(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));

			UI::InputBox("##SceneHierarchySearchBar", width, &m_SearchInputBuffer, ICON_LC_SEARCH " Search...");
		}

		m_IsSearchBarFocused = ImGui::IsItemActive() && ImGui::IsItemFocused();

		// Type bar
		ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0.01f, 0.01f, 0.01f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4(0.01f, 0.01f, 0.01f, 1.0f));
		if (ImGui::BeginTable("TypeBar", 2, ImGuiTableFlags_Borders))
		{
			ImGui::TableSetupColumn(ICON_LC_TAG " Label", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, ImGui::GetWindowWidth() / 4.0f);
			ImGui::TableHeadersRow();
			ImGui::EndTable();
		}
		ImGui::PopStyleColor(2);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);

		{
			UI::ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 4));
			UI::ScopedStyleColor childBg(ImGuiCol_ChildBg, Theme::WindowBg);

			ImGui::BeginChild("##EntityList", ImVec2(-1, -1), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_AlwaysUseWindowPadding);
		}

		if (ImGui::BeginPopupContextWindow((const char*)__null, m_PopupFlags))
		{
			DisplayCreateEntityMenu();
			ImGui::EndPopup();
		}

		ImRect windowRect(ImGui::GetWindowPos(), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y));
		if (ImGui::BeginDragDropTargetCustom(windowRect, ImGui::GetID("##EntityList")))
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_SCENE_HIERARCHY_ENTITY_NODE"))
			{
				const fbentt::entity payloadEntity = *((const fbentt::entity*)payload->Data);
				m_Context->ReparentEntity(payloadEntity, fbentt::null);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);
		m_IsSelectedNodeDisplayed = false;

		DisplayEntityTree(m_Context->GetWorldEntity());

		ImGui::PopStyleVar();

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
			m_SelectionContext = fbentt::null;

		ImGui::EndChild();
		ImGui::End();

		m_InspectorPanel->SetSelectionContext(m_SelectionContext);
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
			m_RenamedEntity = fbentt::null;
		}
		ImGui::PopItemWidth();

		// Remove the input box when it is defocused/deactivated
		if (ImGui::IsItemDeactivated())
			m_RenamedEntity = fbentt::null;
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_InspectorPanel->SetContext(m_Context);
	}

	void SceneHierarchyPanel::SetSelectionContext(fbentt::entity entity)
	{
		m_SelectionContext = entity;
		m_InspectorPanel->SetSelectionContext(m_SelectionContext);
	}

	void SceneHierarchyPanel::DisplayEntityTree(fbentt::entity entity)
	{
		// "Name" of the entity
		auto& tag = m_Context->GetRegistry()->get<TagComponent>(entity).Tag;

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
		const bool isRenamed = m_RenamedEntity == entity;
		const bool isSelected = m_SelectionContext == entity;
		m_IsSelectedNodeDisplayed = m_IsSelectedNodeDisplayed || isSelected;

		bool hasChild = false;

		if (auto* relation = m_Context->GetRegistry()->try_get<RelationshipComponent>(entity))
			hasChild = relation->FirstChild != fbentt::null;

		const int treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_AllowItemOverlap
			| (isSelected ? ImGuiTreeNodeFlags_Selected : 0)
			| (hasChild ? 0 : ImGuiTreeNodeFlags_Leaf)
			| (isRenamed ? 0 : ImGuiTreeNodeFlags_SpanFullWidth);

		ImGui::PushID((const void*)(uint64_t)entity);

		// Set the current entity tree node expanded until the selected node is visible
		if (!m_IsSelectedNodeDisplayed)
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);

		bool open;
		{
			// Styling of the Entity TreeNode
			const float textColor = isSelected ? 0.0f : 1.0f;
			UI::ScopedStyleColor headerColor(ImGuiCol_Header, Theme::AccentColor); // Main Accent Color
			UI::ScopedStyleColor headerActiveColor(ImGuiCol_HeaderActive, Theme::AccentColorLight);
			UI::ScopedStyleColor headerHovered(ImGuiCol_HeaderHovered, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f }, isSelected);
			UI::ScopedStyleColor textC(ImGuiCol_Text, ImVec4{ textColor, textColor, textColor, 1.0f });
			UI::ScopedStyleVariable framePadding(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 2.5f });
			UI::ScopedStyleVariable itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
			UI::ScopedStyleColor textC2(ImGuiCol_Text, ImVec4{ 1.0f, 0.236f, 0.0f, 1.0f }, highlight);

			// Display the actual entity node with it's tag
			open = ImGui::TreeNodeEx((const void*)(uint64_t)entity, treeNodeFlags, "%s %s", isWorldEntity ? ICON_LC_MOUNTAIN_SNOW : ICON_LC_BOX, tag.c_str());
		}

		// Select entity if clicked
		if (ImGui::IsItemClicked())
			m_SelectionContext = entity;

		// World Entity should not be renamed
		if (!isWorldEntity)
		{
			// Check for rename shortcuts being used
			if (isSelected && ImGui::IsWindowFocused())
			{
				ImGuiIO& io = ImGui::GetIO();
				if (!io.KeyMods && ImGui::IsKeyPressed(ImGuiKey_Enter)) // TODO: Shouldn't work with modifier but it does
					m_RenamedEntity = entity;
			}
		}

		// Display Context Menu
		bool shouldDeleteEntity = false, shouldDuplicateEntity = false;

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
				const fbentt::entity payloadEntity = *((const fbentt::entity*)payload->Data);
				m_Context->ReparentEntity(payloadEntity, entity);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::PopID();

		// Rename Entity
		if (isRenamed)
		{
			m_RenameBuffer = tag;
			RenameNode(tag);
		}

		// Display all the children of the entity if the current node is expanded
		if (open)
		{
			if (hasChild)
			{
				fbentt::entity child = m_Context->GetRegistry()->get<RelationshipComponent>(entity).FirstChild;
				while (child != fbentt::null)
				{
					auto temp = m_Context->GetRegistry()->get<RelationshipComponent>(child).NextSibling;
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
			m_SelectionContext = duplicate;
		}

		if (shouldDeleteEntity)
		{
			m_Context->DestroyEntityTree(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = fbentt::null;
		}
	}

	void SceneHierarchyPanel::DisplayCreateEntityMenu(fbentt::entity parent)
	{
		if (ImGui::BeginMenu(ICON_LC_PLUS "\tCreate"))
		{
			if (ImGui::MenuItem(ICON_LC_SQUARE "\tEmpty"))
			{
				const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Empty", parent);
				m_SelectionContext = entity;
			}
			if (ImGui::MenuItem(ICON_LC_TEXT "\tText"))
			{
				const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Text", parent);
				m_Context->GetRegistry()->emplace<TextComponent>(entity);
				m_SelectionContext = entity;
			}
			if (ImGui::MenuItem(ICON_LC_CUBOID "\tMesh"))
			{
				const auto entity = m_Context->CreateEntityWithTagTransformAndParent("StaticMesh", parent);
				m_Context->GetRegistry()->emplace<MeshComponent>(entity);
				m_SelectionContext = entity;
			}
			if (ImGui::MenuItem(ICON_LC_CAMERA "\tCamera"))
			{
				const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Camera", parent);
				m_Context->GetRegistry()->emplace<CameraComponent>(entity);
				m_SelectionContext = entity;
			}
			if (ImGui::BeginMenu("Light"))
			{
				if (ImGui::MenuItem(ICON_LC_SUNRISE "\tSky Light"))
				{
					const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Sky Light", parent);
					m_Context->GetRegistry()->emplace<SkyLightComponent>(entity);
					m_SelectionContext = entity;
				}
				if (ImGui::MenuItem(ICON_LC_SUN "\tDirectional Light"))
				{
					const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Directional Light", parent);
					m_Context->GetRegistry()->emplace<DirectionalLightComponent>(entity);
					m_SelectionContext = entity;
				}
				if (ImGui::MenuItem(ICON_LC_LIGHTBULB "\tPoint Light"))
				{
					const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Point Light", parent);
					m_Context->GetRegistry()->emplace<PointLightComponent>(entity);
					m_SelectionContext = entity;
				}
				if (ImGui::MenuItem(ICON_LC_CONE "\tSpot Light"))
				{
					const auto entity = m_Context->CreateEntityWithTagTransformAndParent("Spot Light", parent);
					m_Context->GetRegistry()->emplace<SpotLightComponent>(entity);
					m_SelectionContext = entity;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
	}

} // namespace Flameberry
