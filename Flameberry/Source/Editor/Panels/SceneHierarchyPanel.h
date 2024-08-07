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
		void SetSelectionContext(fbentt::entity entity);

		fbentt::entity GetSelectionContext() const { return m_SelectionContext; }
		bool IsFocused() const { return m_IsFocused && !m_IsSearchBarFocused && m_RenamedEntity == fbentt::null; }

		void DisplayEntityTree(fbentt::entity entity);
		void DisplayCreateEntityMenu(fbentt::entity parent = (fbentt::entity)fbentt::null);

	private:
		fbentt::entity m_SelectionContext = {};
		fbentt::entity m_RenamedEntity = {};

		Ref<Scene> m_Context;
		Ref<InspectorPanel> m_InspectorPanel;

		static constexpr ImGuiPopupFlags m_PopupFlags = ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight;
		std::string m_RenameBuffer, m_SearchInputBuffer;

		bool m_IsSelectedNodeDisplayed = false, m_IsFocused = false, m_IsSearchBarFocused = false;
	};

} // namespace Flameberry
