#include "EditorLayer.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

namespace Flameberry {
    class FlameEditorApp: public Application
    {
    public:
        FlameEditorApp()
        {
            PushLayer<EditorLayer>();
        }

        ~FlameEditorApp()
        {
        }
    };

    std::shared_ptr<Application> Application::CreateClientApp()
    {
        return std::make_shared<FlameEditorApp>();
    }
}
