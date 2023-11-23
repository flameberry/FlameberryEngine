#include "LauncherLayer.h"
#include "EditorLayer.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

namespace Flameberry {
    class FlameberryEditor : public Application
    {
    public:
        FlameberryEditor(const ApplicationSpecification& specification)
            : Application(specification)
        {
            // TODO: Improve this API urgently
            m_LauncherLayer = new LauncherLayer(FBY_BIND_EVENT_FN(FlameberryEditor::OpenEditor));
            PushLayer(m_LauncherLayer);
        }

        ~FlameberryEditor()
        {
        }
        
        void OpenEditor(const std::shared_ptr<Project>& project)
        {
            auto projectRef = project; // This is to prevent `project` being deleted when Layer is poped
            PopLayer(m_LauncherLayer);
            delete m_LauncherLayer;
            
            auto& window = Application::Get().GetWindow();
            window.SetTitle(fmt::format("Flameberry Engine [{}]", FBY_CONFIG_STR).c_str());
            window.SetSize(1280, 720);
            window.MoveToCenter();
            
            m_EditorLayer = new EditorLayer(projectRef);
            PushLayer(m_EditorLayer);
        }
    private:
        Layer* m_LauncherLayer = nullptr;
        Layer* m_EditorLayer = nullptr;
    };

    Application* Application::CreateClientApp()
    {
        ApplicationSpecification applicationSpec;
        applicationSpec.Name = "Flameberry-Editor";
        applicationSpec.WindowSpec.Width = 1280;
        applicationSpec.WindowSpec.Height = 720;
        
        return new FlameberryEditor(applicationSpec);
    }
}
