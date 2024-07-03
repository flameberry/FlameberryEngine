#include "SceneHierarchyPanel.h"

#include <filesystem>

#include <glm/gtc/type_ptr.hpp>
#include <IconFontCppHeaders/IconsLucide.h>

#include "Core/UI.h"

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

		if (m_IsSearchBarFocused)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));

		UI::InputBox("##SceneHierarchySearchBar", width, m_SearchInputBuffer, 256, ICON_LC_SEARCH " Search...");

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		if (m_IsSearchBarFocused)
		{
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
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

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 4));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::WindowBg);
		ImGui::BeginChild("##EntityList", ImVec2(-1, -1), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_AlwaysUseWindowPadding);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		if (ImGui::BeginPopupContextWindow((const char*)__null, m_PopupFlags))
		{
			DrawCreateEntityMenu();
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
		m_Context->m_Registry->for_each([this](fbentt::entity entity) {
			if (m_Context->IsEntityRoot(entity))
				DrawEntityNode(entity);
		});
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
		strcpy(m_RenameBuffer, tag.c_str());

		ImGui::SameLine();
		ImGui::SetKeyboardFocusHere();
		ImGui::PushItemWidth(-1.0f);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 2.5f });
		if (ImGui::InputText("###Rename", m_RenameBuffer, 256, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
		{
			tag = std::string(m_RenameBuffer);
			m_RenamedEntity = fbentt::null;
		}
		ImGui::PopStyleVar();
		ImGui::PopItemWidth();
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

	void SceneHierarchyPanel::DrawEntityNode(fbentt::entity entity)
	{
		auto& tag = m_Context->m_Registry->get<TagComponent>(entity).Tag;

		bool highlight = false;

		if (m_SearchInputBuffer[0] != '\0')
		{
			// TODO: Maybe some optimisation to not search again if the input string is same
			if (Algorithm::KmpSearch(tag.c_str(), m_SearchInputBuffer, true) != -1)
				highlight = true;
		}

		bool isRenamed = m_RenamedEntity == entity;
		bool isSelected = m_SelectionContext == entity;
		m_IsSelectedNodeDisplayed = m_IsSelectedNodeDisplayed || isSelected;

		bool should_delete_entity = false, should_duplicate_entity = false;
		int treeNodeFlags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0)
			| ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_FramePadding;

		bool hasChild = false;

		if (m_Context->m_Registry->has<RelationshipComponent>(entity))
		{
			auto& relation = m_Context->m_Registry->get<RelationshipComponent>(entity);
			hasChild = relation.FirstChild != fbentt::null;
		}

		if (!hasChild)
			treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;

		if (m_RenamedEntity != entity)
			treeNodeFlags |= ImGuiTreeNodeFlags_SpanFullWidth;

		ImGui::PushID((const void*)(uint64_t)entity);

		const float textColor = isSelected ? 0.0f : 1.0f;
		ImGui::PushStyleColor(ImGuiCol_Header, Theme::AccentColor); // Main Accent Color
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, Theme::AccentColorLight);
		if (isSelected)
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4{ 254.0f / 255.0f, 211.0f / 255.0f, 140.0f / 255.0f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ textColor, textColor, textColor, 1.0f });

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 2.0f, 2.5f });
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		if (!m_IsSelectedNodeDisplayed)
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);

		if (highlight)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.0f, 0.236f, 0.0f, 1.0f });

		bool open = ImGui::TreeNodeEx((const void*)(uint64_t)entity, treeNodeFlags, "%s", tag.c_str());

		if (highlight)
			ImGui::PopStyleColor();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(isSelected ? 4 : 3);

		if (ImGui::IsItemClicked())
			m_SelectionContext = entity;

		if (isSelected && ImGui::IsWindowFocused())
		{
			ImGuiIO& io = ImGui::GetIO();
			if (!io.KeyMods && ImGui::IsKeyPressed(ImGuiKey_Enter)) // TODO: Shouldn't work with modifier but it does
				m_RenamedEntity = entity;
		}

		if (ImGui::BeginPopupContextItem("EntityNodeContextMenu", m_PopupFlags))
		{
			DrawCreateEntityMenu(entity);

			if (ImGui::MenuItem(ICON_LC_TEXT_CURSOR_INPUT "\tRename"))
				m_RenamedEntity = entity;

			if (ImGui::MenuItem(ICON_LC_COPY "\tDuplicate Entity"))
				should_duplicate_entity = true;

			if (ImGui::MenuItem(ICON_LC_DELETE "\tDelete Entity"))
				should_delete_entity = true;

			ImGui::EndPopup();
		}

		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(
				"FBY_SCENE_HIERARCHY_ENTITY_NODE",
				&entity,
				sizeof(entity),
				ImGuiCond_Once);
			ImGui::Text("%s", tag.c_str());
			ImGui::EndDragDropSource();
		}

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

		if (isRenamed)
			RenameNode(tag);

		if (open)
		{
			if (hasChild)
			{
				fbentt::entity child = m_Context->m_Registry->get<RelationshipComponent>(entity).FirstChild;
				while (child != fbentt::null)
				{
					auto temp = m_Context->m_Registry->get<RelationshipComponent>(child).NextSibling;
					DrawEntityNode(child);
					child = temp;
				}
			}
			ImGui::TreePop();
		}

		if (should_duplicate_entity)
		{
			const auto duplicate = m_Context->DuplicateEntity(entity);
			m_SelectionContext = duplicate;
		}

		if (should_delete_entity)
		{
			m_Context->DestroyEntityTree(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = fbentt::null;
		}
	}

	void SceneHierarchyPanel::DrawCreateEntityMenu(fbentt::entity parent)
	{
		if (ImGui::BeginMenu(ICON_LC_PLUS "\tCreate"))
		{
			if (ImGui::MenuItem(ICON_LC_SQUARE "\tEmpty"))
			{
				auto entity = m_Context->CreateEntityWithTagAndParent("Empty", parent);
				m_SelectionContext = entity;
			}
			if (ImGui::MenuItem(ICON_LC_CUBOID "\tMesh"))
			{
				auto entity = m_Context->CreateEntityWithTagAndParent("StaticMesh", parent);
				m_Context->m_Registry->emplace<MeshComponent>(entity);
				m_SelectionContext = entity;
			}
			if (ImGui::MenuItem(ICON_LC_CAMERA "\tCamera"))
			{
				auto entity = m_Context->CreateEntityWithTagAndParent("Camera", parent);
				m_Context->m_Registry->emplace<CameraComponent>(entity);
				m_SelectionContext = entity;
			}
			if (ImGui::BeginMenu("Light"))
			{
				if (ImGui::MenuItem(ICON_LC_SUNRISE "\tSky Light"))
				{
					auto entity = m_Context->CreateEntityWithTagAndParent("Sky Light", parent);
					m_Context->m_Registry->emplace<SkyLightComponent>(entity);
					m_SelectionContext = entity;
				}
				if (ImGui::MenuItem(ICON_LC_SUN "\tDirectional Light"))
				{
					auto entity = m_Context->CreateEntityWithTagAndParent("Directional Light", parent);
					m_Context->m_Registry->emplace<DirectionalLightComponent>(entity);
					m_SelectionContext = entity;
				}
				if (ImGui::MenuItem(ICON_LC_LIGHTBULB "\tPoint Light"))
				{
					auto entity = m_Context->CreateEntityWithTagAndParent("Point Light", parent);
					m_Context->m_Registry->emplace<PointLightComponent>(entity);
					m_SelectionContext = entity;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
	}
} // namespace Flameberry
