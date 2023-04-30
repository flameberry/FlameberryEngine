#include "EditorLayer.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

namespace Flameberry {
    class FlameberryEditor : public Application
    {
    public:
        FlameberryEditor()
        {
            PushLayer<EditorLayer>();
        }

        ~FlameberryEditor()
        {
        }
    };

    std::shared_ptr<Application> Application::CreateClientApp()
    {
        return std::make_shared<FlameberryEditor>();
    }
}