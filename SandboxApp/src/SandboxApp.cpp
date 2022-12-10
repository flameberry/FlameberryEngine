#include "SandboxApp.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

SandboxApp::SandboxApp()
{
    Flameberry::VulkanRenderer::Init(Flameberry::Application::Get().GetWindow().GetGLFWwindow());

    Flameberry::PerspectiveCameraInfo cameraInfo{};
    cameraInfo.aspectRatio = Flameberry::Application::Get().GetWindow().GetWidth() / Flameberry::Application::Get().GetWindow().GetHeight();
    cameraInfo.FOV = 45.0f;
    cameraInfo.zNear = 0.1f;
    cameraInfo.zFar = 1000.0f;
    cameraInfo.cameraPostion = glm::vec3(0.0f, 0.0f, 4.0f);
    cameraInfo.cameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);

    m_ActiveCamera = Flameberry::PerspectiveCamera(cameraInfo);
}

SandboxApp::~SandboxApp()
{
    Flameberry::VulkanRenderer::CleanUp();
}

void SandboxApp::OnUpdate(float delta)
{
    m_ActiveCamera.OnUpdate(delta);
    Flameberry::VulkanRenderer::RenderFrame(m_ActiveCamera);
}

void SandboxApp::OnUIRender()
{
}

std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
{
    return std::make_shared<SandboxApp>();
}