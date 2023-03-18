#include "EditorLayer.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

namespace Flameberry {
    class FlameberryEditorApp: public Application
    {
    public:
        FlameberryEditorApp()
        {
            PushLayer<EditorLayer>();
        }

        ~FlameberryEditorApp()
        {
        }
    };

    std::shared_ptr<Application> Application::CreateClientApp()
    {
        return std::make_shared<FlameberryEditorApp>();
    }
}
