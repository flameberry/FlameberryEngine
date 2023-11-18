#include "LauncherLayer.h"

#include "UI.h"

namespace Flameberry {
    
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
        
        if (ImGui::BeginPopupModal("New Project", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            UI::InputBox("##ProjectNameInput", 0.0f, m_ProjectNameBuffer, 128, "Project Name...");
            UI::InputBox("##ProjectPathInput", 0.0f, m_ProjectPathBuffer, 512, "Enter a directory...");
            ImGui::SameLine();
            if (ImGui::Button("..."))
            {
                std::string directoryPath = platform::OpenFolder();
                if (!directoryPath.empty())
                    strcpy(m_ProjectPathBuffer, directoryPath.c_str());
                FBY_LOG("Wow you are lazy, just type in a path!");
            }
            
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::SameLine();
            if (ImGui::Button("Create", ImVec2(120, 0))) 
            {
                // If a new project is to be created then
                // ProjectConfig projectConfig;
                // projectConfig.Name = "SandboxProject";
                // projectConfig.AssetDirectory = "Assets";
                // projectConfig.ScriptAssemblyPath = fmt::format("Binaries/net7.0/{}.dll", projectConfig.Name);
                
                // m_Project = Project::Create(FBY_PROJECT_DIR"SandboxProject", projectConfig);
                g_ShouldClose = true;
            }
            ImGui::SetItemDefaultFocus();
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
    
    void LauncherLayer::OnEvent(Event &e) 
    {
    }
    
    void LauncherLayer::OnDestroy() {
    }
    
}
