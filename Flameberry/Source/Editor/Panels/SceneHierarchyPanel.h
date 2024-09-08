#pragma once

#include "Flameberry.h"
#include "InspectorPanel.h"

namespace Flameberry {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel(const Ref<Scene>& context);
		~SceneHierarchyPanel();

		void OnUIRender();

		void RenameNode(std::string& tag);
		void SetContext(const Ref<Scene>& context);
		void SetSelectionContext(FEntity entity);

		FEntity GetSelectionContext() const { return m_SelectionContext; }
		bool IsFocused() const { return m_IsFocused && !m_IsSearchBarFocused && m_RenamedEntity == FEntity::Null; }

		void DisplayEntityTree(FEntity entity);
		void DisplayCreateEntityMenu(FEntity parent = FEntity::Null);

	private:
		/**
		 * This function is present in `SceneHierarchyPanel` and not in `Scene` as the usage of CollectionComponent is meant for Editor Only
		 */
		FEntity CreateCollectionEntity(const std::string& name, FEntity parent);

	private:
		FEntity m_SelectionContext = {};
		FEntity m_RenamedEntity = {};

		Ref<Scene> m_Context;
		Ref<InspectorPanel> m_InspectorPanel;

		static constexpr ImGuiPopupFlags m_PopupFlags = ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight;
		std::string m_RenameBuffer, m_SearchInputBuffer;

		bool m_IsSelectedNodeDisplayed = false, m_IsFocused = false, m_IsSearchBarFocused = false;
	};

} // namespace Flameberry
