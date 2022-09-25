// #include "Flameberry.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

// class SandboxApp : public Flameberry::Application
// {
// public:
//     SandboxApp()
//     {
//         m_Window = Flameberry::Window::Create(1280, 720, "Sandbox App");
//         Flameberry::VulkanRenderer::Init(m_Window->GetGLFWwindow());
//     }

//     virtual ~SandboxApp()
//     {
//         Flameberry::VulkanRenderer::CleanUp();
//     }

//     void OnUpdate(float delta) override
//     {
//         Flameberry::VulkanRenderer::RenderFrame();
//     }

//     void OnUIRender() override
//     {
//     }
// private:
//     std::shared_ptr<Flameberry::Window> m_Window;
// };

// std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
// {
//     return std::make_shared<SandboxApp>();
// }