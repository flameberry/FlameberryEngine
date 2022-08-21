#include "Flameberry.h"

class SandboxApp : public Flameberry::Application
{
public:
    SandboxApp() {}
    virtual ~SandboxApp() {}
    void OnRender() override
    {
        FL_LOG("Rendering [SANDBOXAPP]");
    }
    void OnUIRender() override
    {
        FL_LOG("Rendering UI [SANDBOXAPP]");
    }
};

std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
{
    return std::make_shared<SandboxApp>();
}