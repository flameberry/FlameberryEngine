#include "LauncherLayer.h"

#include <fstream>

#include "UI.h"

namespace Flameberry {

    static const std::string g_CSProjTemplate = 
R"(
<Project Sdk="Microsoft.NET.Sdk">
<PropertyGroup>
    <TargetFramework>net7.0</TargetFramework>
    <ImplicitUsings>enable</ImplicitUsings>
    <Nullable>enable</Nullable>
    <OutputPath>Binaries</OutputPath>
</PropertyGroup>
<ItemGroup>
    <Reference Include="Flameberry-ScriptCore">
    <HintPath>/Users/flameberry/Developer/FlameberryEngine/Flameberry-ScriptCore/bin/Debug/net7.0/Flameberry-ScriptCore.dll</HintPath>
    <Private>True</Private>
    </Reference>
</ItemGroup>
</Project>
)";

    static bool g_ShouldClose = false;

    LauncherLayer::LauncherLayer(const std::function<void(const std::shared_ptr<Project>&)>& callback)
        : m_Callback(callback)
    {
    }

    void LauncherLayer::OnCreate()
    {
        auto& window = Application::Get().GetWindow();
        window.SetTitle("Launcher Window");
        window.SetSize(1280 / 2, 720 / 2);
    }

    void LauncherLayer::OnUpdate(float delta)
    {
        if (g_ShouldClose)
            m_Callback(m_Project);
    }

    void LauncherLayer::OnUIRender()
    {
        ImGui::Begin("Project Launcher");
        if (ImGui::Button("New Project"))
            ImGui::OpenPopup("New Project");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("New Project", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGuiIO& io = ImGui::GetIO();
            auto& bigFont = io.Fonts->Fonts[0];

            ImGui::PushFont(bigFont);
            ImGui::Text("Enter the full project path:");
            ImGui::PopFont();

            UI::InputBox("##ProjectNameInput", 150.0f, m_ProjectNameBuffer, 128, "Project Name...");
            UI::InputBox("##ProjectPathInput", 150.0f, m_ProjectPathBuffer, 512, "Enter a directory...");
            ImGui::SameLine();
            if (ImGui::Button("..."))
            {
                std::string directoryPath = platform::OpenFolder();
                if (!directoryPath.empty())
                    strcpy(m_ProjectPathBuffer, directoryPath.c_str());
                FBY_LOG("Wow you are lazy, just type in a path!");
            }

            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                std::filesystem::path projectParentPath(m_ProjectPathBuffer);
                if (!projectParentPath.empty() && std::filesystem::exists(projectParentPath))
                {
                    if (strlen(m_ProjectNameBuffer))
                    {
                        ProjectConfig projectConfig;
                        projectConfig.Name = std::string(m_ProjectNameBuffer);
                        projectConfig.AssetDirectory = "Assets";
                        projectConfig.ScriptAssemblyPath = fmt::format("Binaries/net7.0/{}.dll", projectConfig.Name);

                        m_Project = Project::Create(projectParentPath / projectConfig.Name, projectConfig);

                        // Setup
                        // 1. Create the project directory
                        if (!std::filesystem::exists(m_Project->GetProjectDirectory()))
                            std::filesystem::create_directory(m_Project->GetProjectDirectory());

                        if (std::filesystem::is_empty(m_Project->GetProjectDirectory()))
                        {
                            // 2. Serialize the Project
                            m_Project->Save();
                            // 3. Create an Assets folder
                            std::filesystem::create_directory(m_Project->GetAssetDirectory());
                            // 4. Write a .csproj file with template
                            std::string csprojPath = m_Project->GetProjectDirectory() / fmt::format("{}.csproj", projectConfig.Name);
                            std::ofstream csprojFile(csprojPath, std::ios::out | std::ios::trunc);

                            FBY_ASSERT(csprojFile.is_open(), "Failed to open ofstream to: {}", csprojPath);
                            csprojFile << g_CSProjTemplate;
                            csprojFile.close();

                            // Signal callback to FlameberryEditor class
                            g_ShouldClose = true;
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s is not empty!", m_Project->GetProjectDirectory().c_str());
                            FBY_ERROR("Failed to create project: Project Directory: {} is not empty!", m_Project->GetProjectDirectory());
                        }
                    }
                    else
                    {
                        FBY_ERROR("Failed to create project: Project name is empty!");
                    }
                }
                else
                {
                    FBY_ERROR("Failed to create project: Either project path is empty or it does not exist!");
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        if (ImGui::Button("Open Project"))
        {
            // Open a project browser window and if an existing project is selected then...
            std::string path = platform::OpenFile("Flameberry Project File (*.fbproj)\0.fbproj\0");
            if (!path.empty())
            {
                m_Project = ProjectSerializer::DeserializeIntoNewProject(path);
                g_ShouldClose = true;
            }
        }
        ImGui::End();
    }

    void LauncherLayer::OnEvent(Event& e)
    {
    }

    void LauncherLayer::OnDestroy() {
    }

}
