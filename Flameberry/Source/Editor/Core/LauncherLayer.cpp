#include "LauncherLayer.h"

#include <IconFontCppHeaders/IconsLucide.h>

#include "Core/UI.h"
#include "ImGui/Theme.h"
#include "Project/ProjectRegistry.h"

namespace Flameberry {

	LauncherLayer::LauncherLayer(const std::function<void(const Ref<Project>&)>& callback)
		: m_OpenProjectCallback(callback) {}

	void LauncherLayer::OnCreate() {
		auto& window = Application::Get().GetWindow();
		constexpr int width = 1280 * 0.7f, height = 720 * 0.7f;

		window.SetTitle("Launcher Window");
		window.SetSize(width, height);
		window.MoveToCenter();

		m_FlameberryIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR "Flameberry/Assets/Icons/FlameberryIcon.png");

		// Load User Projects
		m_ProjectRegistry = ProjectRegistryManager::LoadEntireProjectRegistry();
		FBY_LOG("Found user projects: {}", m_ProjectRegistry.size());
	}

	void LauncherLayer::OnUpdate(float delta) {
		if (m_ShouldClose) m_OpenProjectCallback(m_Project);
	}

	void LauncherLayer::OnUIRender() {
		if (!Application::Get().GetWindow().GetSpecification().NativeTitlebar) UI_Titlebar();

		// The launcher fills the window below the titlebar instead of being a dockable panel
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;
		{
			UI::ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			UI::ScopedStyleVariable windowBorderSize(ImGuiStyleVar_WindowBorderSize, 0.0f);
			UI::ScopedStyleVariable windowRounding(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::Begin("Project Launcher", nullptr, windowFlags);
		}

		UI_Sidebar();
		ImGui::SameLine(0.0f, 0.0f);
		UI_RecentProjects();

		if (m_ShouldOpenNewProjectPopup) {
			ImGui::OpenPopup("New Project");
			m_ShouldOpenNewProjectPopup = false;
		}
		UI_NewProjectPopup();

		ImGui::End();
	}

	void LauncherLayer::OnEvent(Event& e) {}

	void LauncherLayer::OnDestroy() {}

	void LauncherLayer::UI_Titlebar() {
		const auto& specification = Application::Get().GetWindow().GetSpecification();
		const glm::vec4& color = specification.TitlebarGradientColor;
		if (UI::BeginTitlebar(specification.TitlebarHeight, ImVec4(color.r, color.g, color.b, color.a))) {
			constexpr const char* title = "Project Launcher";
			ImGui::SetCursorPosX(0.5f * (ImGui::GetWindowWidth() - ImGui::CalcTextSize(title).x));

			UI::ScopedStyleColor textColor(ImGuiCol_Text, Theme::TextMuted);
			ImGui::TextUnformatted(title);
		}
		UI::EndTitlebar();
	}

	void LauncherLayer::UI_Sidebar() {
		constexpr float sidebarWidth = 220.0f, padding = 16.0f, buttonHeight = 34.0f;

		{
			UI::ScopedStyleColor childBg(ImGuiCol_ChildBg, Theme::WindowBgDark);
			UI::ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
			ImGui::BeginChild("##LauncherSidebar", ImVec2(sidebarWidth, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
		}

		// Branding
		constexpr float logoSize = 40.0f;
		ImGui::Image(
			reinterpret_cast<ImTextureID>(m_FlameberryIcon->CreateOrGetImGuiDescriptorSet()), ImVec2(logoSize, logoSize)
		);
		ImGui::SameLine();
		{
			const float textBlockHeight = 2.0f * ImGui::GetFontSize() + 2.0f;
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 0.5f * (logoSize - textBlockHeight - 2.0f));
			ImGui::BeginGroup();
			ImGui::PushFont(nullptr, 17.0f);
			ImGui::TextUnformatted("Flameberry");
			ImGui::PopFont();
			UI::ScopedStyleColor textColor(ImGuiCol_Text, Theme::TextMuted);
			ImGui::TextUnformatted("Engine");
			ImGui::EndGroup();
		}

		ImGui::Dummy(ImVec2(0.0f, 12.0f));

		const float buttonWidth = ImGui::GetContentRegionAvail().x;
		if (UI::PrimaryButton(ICON_LC_PLUS "  New Project", ImVec2(buttonWidth, buttonHeight))) {
			m_ProjectNameBuffer.clear();
			m_NewProjectError.clear();
			m_ShouldOpenNewProjectPopup = true;
		}

		if (ImGui::Button(ICON_LC_FOLDER_OPEN "  Open Project", ImVec2(buttonWidth, buttonHeight)))
			OpenProjectFromFileDialog();

		// Housekeeping action, kept out of the way at the bottom of the sidebar
		ImGui::SetCursorPosY(ImGui::GetWindowHeight() - padding - ImGui::GetFrameHeight());
		{
			UI::ScopedStyleColor button(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			UI::ScopedStyleColor textColor(ImGuiCol_Text, Theme::TextMuted);
			if (ImGui::Button(ICON_LC_ERASER "  Clear Missing Projects", ImVec2(buttonWidth, 0.0f))) {
				ProjectRegistryManager::ClearAllNonExistingProjectRegistryEntries();
				m_ProjectRegistry = ProjectRegistryManager::LoadEntireProjectRegistry();
			}
		}

		ImGui::EndChild();
	}

	void LauncherLayer::UI_RecentProjects() {
		constexpr float padding = 20.0f;
		{
			UI::ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
			ImGui::BeginChild("##RecentProjects", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
		}

		ImGui::PushFont(nullptr, 18.0f);
		ImGui::TextUnformatted("Recent Projects");
		ImGui::PopFont();

		// Project count, aligned to the right end of the header
		{
			const std::string count =
				fmt::format("{} {}", m_ProjectRegistry.size(), m_ProjectRegistry.size() == 1 ? "project" : "projects");
			ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(count.c_str()).x);
			UI::ScopedStyleColor textColor(ImGuiCol_Text, Theme::TextMuted);
			ImGui::TextUnformatted(count.c_str());
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (m_ProjectRegistry.empty()) {
			ImGui::TextDisabled("No recent projects. Create a new project or open an existing one.");
		} else {
			UI::ScopedStyleVariable itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
			for (const auto& entry : m_ProjectRegistry) {
				const std::string projectFilePathStr = entry.ProjectFilePath.string();
				if (UI::ProjectRegistryEntryItem(
						entry.ProjectName.c_str(), projectFilePathStr.c_str(),
						!std::filesystem::exists(entry.ProjectFilePath)
					)) {
					// Open Project
					m_Project = Project::Load(entry.ProjectFilePath);
					m_ShouldClose = true;
				}
			}
		}

		ImGui::EndChild();
	}

	void LauncherLayer::UI_NewProjectPopup() {
		constexpr float popupWidth = 440.0f, buttonWidth = 100.0f;

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(popupWidth, 0.0f));

		bool isOpen;
		{
			UI::ScopedStyleVariable windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));
			isOpen = ImGui::BeginPopupModal(
				"New Project", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
			);
		}
		if (!isOpen) return;

		ImGui::PushFont(nullptr, 18.0f);
		ImGui::TextUnformatted("New Project");
		ImGui::PopFont();
		{
			UI::ScopedStyleColor textColor(ImGuiCol_Text, Theme::TextMuted);
			ImGui::TextUnformatted("Choose a name and where to create the project.");
		}

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		auto fieldLabel = [](const char* label) {
			UI::ScopedStyleColor textColor(ImGuiCol_Text, Theme::TextMuted);
			ImGui::TextUnformatted(label);
		};

		fieldLabel("Name");
		if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
		m_IsProjectNameInputFocused =
			UI::InputBox("##ProjectNameInput", -1.0f, &m_ProjectNameBuffer, "MyProject", m_IsProjectNameInputFocused);

		ImGui::Spacing();

		fieldLabel("Location");
		const float browseButtonSize = ImGui::GetFrameHeight();
		const float pathInputWidth = ImGui::GetContentRegionAvail().x - browseButtonSize - ImGui::GetStyle().ItemSpacing.x;
		m_IsProjectPathInputFocused = UI::InputBox(
			"##ProjectPathInput", pathInputWidth, &m_ProjectPathBuffer, "Choose a directory...", m_IsProjectPathInputFocused
		);
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - browseButtonSize);
		if (ImGui::Button(ICON_LC_FOLDER_OPEN, ImVec2(browseButtonSize, browseButtonSize))) {
			if (const std::string directoryPath = Platform::OpenFolder(); !directoryPath.empty())
				m_ProjectPathBuffer = directoryPath;
		}

		// Show where the project will end up, so that a wrong location is easy to spot before creating it
		if (!m_ProjectNameBuffer.empty() && !m_ProjectPathBuffer.empty()) {
			const std::string projectPath = (std::filesystem::path(m_ProjectPathBuffer) / m_ProjectNameBuffer).string();
			UI::ScopedStyleColor textColor(ImGuiCol_Text, Theme::TextMuted);
			ImGui::TextWrapped("Will be created at %s", projectPath.c_str());
		}

		if (!m_NewProjectError.empty()) {
			UI::ScopedStyleColor textColor(ImGuiCol_Text, Theme::ErrorColor);
			ImGui::TextWrapped("%s", m_NewProjectError.c_str());
		}

		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		// Actions, aligned to the right
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 2.0f * buttonWidth - ImGui::GetStyle().ItemSpacing.x);
		if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			ImGui::CloseCurrentPopup();

		ImGui::SameLine();

		ImGui::BeginDisabled(m_ProjectNameBuffer.empty() || m_ProjectPathBuffer.empty());
		if (UI::PrimaryButton("Create", ImVec2(buttonWidth, 0.0f))) CreateNewProject();
		ImGui::EndDisabled();

		ImGui::EndPopup();
	}

	void LauncherLayer::OpenProjectFromFileDialog() {
		// Open a project browser window and if an existing project is selected then...
		std::string path = Platform::OpenFile("Flameberry Project File (*.fbproj)\0.fbproj\0");
		if (path.empty()) return;

		m_Project = Project::Load(path);

		ProjectRegistryEntry entry{ m_Project->GetConfig().Name, path };
		if (std::find(m_ProjectRegistry.begin(), m_ProjectRegistry.end(), entry) == m_ProjectRegistry.end()) {
			// The project didn't exist before in the ProjectRegistry
			ProjectRegistryManager::AppendEntryToGlobalRegistry(m_Project.get());
		}

		m_ShouldClose = true;
	}

	void LauncherLayer::CreateNewProject() {
		const std::filesystem::path projectParentPath(m_ProjectPathBuffer);
		if (!std::filesystem::is_directory(projectParentPath)) {
			m_NewProjectError = "The location doesn't exist.";
			return;
		}

		if (!(m_Project = Project::CreateProjectOnDisk(projectParentPath, m_ProjectNameBuffer))) {
			m_NewProjectError =
				fmt::format("{} already exists and isn't empty.", (projectParentPath / m_ProjectNameBuffer).string());
			return;
		}

		// Finally add project entry to GlobalProjectRegistry
		ProjectRegistryManager::AppendEntryToGlobalRegistry(m_Project.get());

		// Signal callback to EditorApplication class
		m_ShouldClose = true;
		ImGui::CloseCurrentPopup();
	}

} // namespace Flameberry
