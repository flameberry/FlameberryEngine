#pragma once

#include "Flameberry.h"
#include "Project/Project.h"
#include "Project/ProjectRegistry.h"

namespace Flameberry {

	class LauncherLayer : public Layer {
	public:
		LauncherLayer(const std::function<void(const Ref<Project>&)>& callback);
		virtual ~LauncherLayer() = default;

		void OnCreate() override;
		void OnUpdate(float delta) override;
		void OnUIRender() override;
		void OnEvent(Event& e) override;
		void OnDestroy() override;

	private:
		void UI_Titlebar();
		void UI_Sidebar();
		void UI_RecentProjects();
		void UI_NewProjectPopup();

		void OpenProjectFromFileDialog();
		void CreateNewProject();

	private:
		ProjectRegistry m_ProjectRegistry;

		Ref<Project> m_Project;
		std::function<void(const Ref<Project>&)> m_OpenProjectCallback;
		bool m_ShouldClose = false;

		Ref<Texture2D> m_FlameberryIcon;

		// New Project Popup
		bool m_ShouldOpenNewProjectPopup = false;
		bool m_IsProjectNameInputFocused = false, m_IsProjectPathInputFocused = false;
		std::string m_ProjectNameBuffer, m_ProjectPathBuffer, m_NewProjectError;
	};

} // namespace Flameberry
