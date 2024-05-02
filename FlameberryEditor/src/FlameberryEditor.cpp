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
            // Check for startup project
            if (specification.CommandLineArgs.Count > 1)
            {
                std::filesystem::path projectPath = specification.CommandLineArgs[1];

                if (!std::filesystem::exists(projectPath))
                {
                    FBY_ERROR("Invalid project path passed as command line arguments: {}", projectPath);

                    // TODO: Improve this API urgently
                    m_LauncherLayer = new LauncherLayer(FBY_BIND_EVENT_FN(FlameberryEditor::OpenProjectWithEditor));
                    PushLayer(m_LauncherLayer);
                }
                else
                {
                    auto project = ProjectSerializer::DeserializeIntoNewProject(projectPath);

                    // Set the secondary title of the window to be the name of the project
                    auto& window = Application::Get().GetWindow();
                    window.SetSecondaryTitle(project->GetConfig().Name.c_str());

                    // Create an Editor Instance and push it to Application Layers
                    m_EditorLayer = new EditorLayer(project);
                    PushLayer(m_EditorLayer);
                }
            }
            else
            {
                // TODO: Improve this API urgently
                m_LauncherLayer = new LauncherLayer(FBY_BIND_EVENT_FN(FlameberryEditor::OpenProjectWithEditor));
                PushLayer(m_LauncherLayer);
            }
        }

        ~FlameberryEditor()
        {
        }

        void OpenProjectWithEditor(const Ref<Project>& project)
        {
            auto projectRef = project; // This is to prevent `project` being deleted when Layer is poped

            // Remove the Launcher Window Layer
            PopAndDeleteLayer(m_LauncherLayer);

            // Resize the window from the previous launcher size to the new editor suitable size
            auto& window = Application::Get().GetWindow();
            window.SetTitle(fmt::format("Flameberry Engine [{}]", FBY_CONFIG_STR).c_str());
            window.SetSecondaryTitle(projectRef->GetConfig().Name.c_str());
            window.SetSize(1280, 800);
            window.MoveToCenter();

            // Create an Editor Instance and push it to Application Layers
            m_EditorLayer = new EditorLayer(projectRef);
            PushLayer(m_EditorLayer);
        }
    private:
        Layer* m_LauncherLayer = nullptr;
        Layer* m_EditorLayer = nullptr;
    };

    Application* Application::CreateClientApp(const ApplicationCommandLineArgs& appCmdLineArgs)
    {
        ApplicationSpecification applicationSpec;
        applicationSpec.Name = "Flameberry-Editor";
        applicationSpec.WindowSpec.Width = 1280;
        applicationSpec.WindowSpec.Height = 800;
        applicationSpec.WorkingDirectory = FBY_PROJECT_DIR;
        applicationSpec.CommandLineArgs = appCmdLineArgs;

        return new FlameberryEditor(applicationSpec);
    }
}
