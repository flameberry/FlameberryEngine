#include "EditorLayer.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

namespace Flameberry {
    class FlameberryEditor : public Application
    {
    public:
        FlameberryEditor(const std::string_view& projectPath)
            : m_ProjectPath(projectPath)
        {
            std::filesystem::current_path(m_ProjectPath);
            PushLayer<EditorLayer>(m_ProjectPath);
        }

        ~FlameberryEditor()
        {
        }
    private:
        std::string_view m_ProjectPath;
    };

    std::shared_ptr<Application> Application::CreateClientApp()
    {
        return std::make_shared<FlameberryEditor>(FL_PROJECT_DIR"SandboxApp");
    }
}
