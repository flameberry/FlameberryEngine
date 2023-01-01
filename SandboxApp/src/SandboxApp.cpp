#include "SandboxApp.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"
#include "Application.h"

SandboxApp::SandboxApp()
{
    Flameberry::VulkanRenderer::Init(Flameberry::Application::Get().GetWindow().GetGLFWwindow());
}

SandboxApp::~SandboxApp()
{
    Flameberry::VulkanRenderer::CleanUp();
}

void SandboxApp::OnUpdate(float delta)
{
    Flameberry::VulkanRenderer::RenderFrame();
}

void SandboxApp::OnUIRender()
{
}

std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
{
    return std::make_shared<SandboxApp>();
}