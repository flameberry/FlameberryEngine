#include "LauncherLayer.h"

#include <fstream>

#include "UI.h"

namespace Flameberry {

    LauncherLayer::LauncherLayer(const std::function<void(const Ref<Project>&)>& callback)
        : m_OpenProjectCallback(callback)
    {
    }

    void LauncherLayer::OnCreate()
    {
        auto& window = Application::Get().GetWindow();
        constexpr int width = 1280 * 0.7f, height = 720 * 0.7f;

        window.SetTitle("Launcher Window");
        window.SetSize(width, height);
        window.MoveToCenter();

        // Load User Projects
        m_ProjectRegistry = ProjectRegistryManager::LoadEntireProjectRegistry();
        FBY_LOG("Found user projects: {}", m_ProjectRegistry.size());
    }

    void LauncherLayer::OnUpdate(float delta)
    {
        if (m_ShouldClose)
            m_OpenProjectCallback(m_Project);
    }

    void LauncherLayer::OnUIRender()
    {
        constexpr ImVec2 buttonSize{ 200.0f, 40.0f };
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Project Launcher");
        ImGui::PopStyleVar();

        if (ImGui::Button("Clear Deleted Projects"))
        {
            ProjectRegistryManager::ClearAllNonExistingProjectRegistryEntries();
            m_ProjectRegistry = ProjectRegistryManager::LoadEntireProjectRegistry();
        }

        static float firstChildSize = 100.0f, secondChildSize = 220.0f;
        firstChildSize = ImGui::GetContentRegionAvail().x - secondChildSize - 8.0f;

        ImGui::BeginChild("##ProjectList", ImVec2(firstChildSize, 0), ImGuiChildFlags_AlwaysAutoResize, ImGuiWindowFlags_AlwaysUseWindowPadding);

        for (const auto& entry : m_ProjectRegistry)
        {
            if (UI::ProjectRegistryEntryItem(entry.ProjectName.c_str(), entry.ProjectFilePath.c_str(), !std::filesystem::exists(entry.ProjectFilePath)))
            {
                // Open Project
                m_Project = ProjectSerializer::DeserializeIntoNewProject(entry.ProjectFilePath);
                m_ShouldClose = true;
            }
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##ProjectControls", ImVec2(secondChildSize, 0), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_Border, ImGuiWindowFlags_AlwaysUseWindowPadding);

        if (UI::AlignedButton("New Project", buttonSize))
            ImGui::OpenPopup("New Project");

        UI_NewProjectPopup();

        if (UI::AlignedButton("Open Project", buttonSize))
        {
            // Open a project browser window and if an existing project is selected then...
            std::string path = Platform::OpenFile("Flameberry Project File (*.fbproj)\0.fbproj\0");
            if (!path.empty())
            {
                m_Project = ProjectSerializer::DeserializeIntoNewProject(path);

                ProjectRegistryEntry entry{ m_Project->GetConfig().Name, path };
                if (std::find(m_ProjectRegistry.begin(), m_ProjectRegistry.end(), entry) == m_ProjectRegistry.end())
                {
                    // The project didn't exist before in the ProjectRegistry
                    ProjectRegistryManager::AppendEntryToGlobalRegistry(m_Project.get());
                }

                m_ShouldClose = true;
            }
        }
        ImGui::EndChild();
        ImGui::End();
    }

    void LauncherLayer::OnEvent(Event& e)
    {
    }

    void LauncherLayer::OnDestroy() {
    }

    void LauncherLayer::UI_NewProjectPopup()
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(1280 / 3, 720 / 4));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
        const bool beginPopupModal = ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleVar();

        if (beginPopupModal)
        {
            ImGuiIO& io = ImGui::GetIO();
            auto& bigFont = io.Fonts->Fonts[0];

            ImGui::PushFont(bigFont);
            ImGui::Text("Enter project name and parent directory:");
            ImGui::Spacing();
            ImGui::PopFont();

            const float inputBoxWidth = ImGui::GetContentRegionAvail().x - 30.0f;

            UI::InputBox("##ProjectNameInput", inputBoxWidth, m_ProjectNameBuffer, 128, "Project Name...");
            UI::InputBox("##ProjectPathInput", inputBoxWidth, m_ProjectPathBuffer, 512, "Enter a directory...");
            ImGui::SameLine();
            if (ImGui::Button("..."))
            {
                std::string directoryPath = Platform::OpenFolder();
                if (!directoryPath.empty())
                    strcpy(m_ProjectPathBuffer, directoryPath.c_str());
            }

            ImGui::Spacing();

            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                std::filesystem::path projectParentPath(m_ProjectPathBuffer);
                if (!projectParentPath.empty() && std::filesystem::exists(projectParentPath))
                {
                    if (strlen(m_ProjectNameBuffer))
                    {
                        if (m_Project = Project::CreateProjectOnDisk(projectParentPath, m_ProjectNameBuffer); m_Project)
                            // Signal callback to FlameberryEditor class
                            m_ShouldClose = true;
                        else
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s is not empty!", m_Project->GetProjectDirectory().c_str());
                    }
                    else
                        FBY_ERROR("Failed to create project: Project name is empty!");
                }
                else
                    FBY_ERROR("Failed to create project: Either project path is empty or it does not exist!");
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }

}
