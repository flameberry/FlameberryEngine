#include "EditorLayer.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

namespace Flameberry {
    class FlameberryEditor : public Application
    {
    public:
        FlameberryEditor(const std::shared_ptr<Project>& project)
            : m_Project(project)
        {
            Project::SetActive(m_Project);
            std::filesystem::current_path(m_Project->GetProjectDirectory());
            PushLayer<EditorLayer>(m_Project);
        }

        ~FlameberryEditor()
        {
        }
    private:
        std::shared_ptr<Project> m_Project;
    };

    Application* Application::CreateClientApp()
    {
        ProjectConfig projectConfig;
        projectConfig.AssetDirectory = "Assets";
        
        auto project = Project::Create(FL_PROJECT_DIR"SandboxProject", projectConfig);
        return new FlameberryEditor(project);
    }
}
