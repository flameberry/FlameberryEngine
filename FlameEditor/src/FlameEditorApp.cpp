#include "FlameEditorApp.h"

FlameEditorApp::FlameEditorApp()
    : m_Framebuffer(Flameberry::Framebuffer::Create()),
    m_ViewportSize(1280, 720),
    m_Camera({ 1280.0f, 720.0f }, 1.0f),
    m_Renderer3D(Flameberry::Renderer3D::Create())
{
    Flameberry::Renderer2DInitInfo rendererInitInfo{};
    rendererInitInfo.userWindow = Flameberry::Application::Get().GetWindow().GetGLFWwindow();
    rendererInitInfo.enableCustomViewport = true;
    rendererInitInfo.customViewportSize = { 1280.0f, 720.0f };

    m_Renderer2D = m_Renderer2D->Create();
    m_Renderer2D->Init(rendererInitInfo);

    // Dealing with 3D Renderer
    // m_Renderer3D->Init(Flameberry::Application::Get().GetWindow().GetGLFWwindow());

    Flameberry::PerspectiveCameraInfo cameraInfo{};
    cameraInfo.aspectRatio = 1280.0f / 720.0f;
    cameraInfo.FOV = 45.0f;
    cameraInfo.cameraPostion = glm::vec3(0, 0, 2);
    cameraInfo.cameraDirection = glm::vec3(0, 0, -1);

    m_PerspectiveCamera = Flameberry::PerspectiveCamera(cameraInfo);
}

FlameEditorApp::~FlameEditorApp()
{
    m_Renderer3D->CleanUp();
    m_Renderer2D->CleanUp();
}

void FlameEditorApp::OnUpdate(float delta)
{
    Flameberry::ScopedTimer timer(&m_LastRenderTime);

    // Framebuffer Resize
    if (glm::vec2 framebufferSize = m_Framebuffer->GetFramebufferSize();
        m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
        (framebufferSize.x != m_ViewportSize.x || framebufferSize.y != m_ViewportSize.y))
    {
        m_Framebuffer->SetFramebufferSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        m_Framebuffer->OnUpdate();
    }

    m_Framebuffer->Bind();
    m_Renderer2D->SetCustomViewportSize(m_ViewportSize);

    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // 3D
    // m_PerspectiveCamera.SetAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
    // m_PerspectiveCamera.OnUpdate();

    // m_Renderer3D->Begin(m_PerspectiveCamera);
    // m_Renderer3D->OnDraw();
    // m_Renderer3D->End();

    // 2D
    m_Camera.SetViewportSize(m_ViewportSize);

    m_Renderer2D->Begin(m_Camera);
    m_Renderer2D->AddQuad({ 0.0f,  0.0f, 0.0f }, { 0.2f, 0.2f }, FL_PINK);
    m_Renderer2D->AddQuad({ 0.2f,  0.0f, 0.0f }, { 0.2f, 0.2f }, FL_PROJECT_DIR"SandboxApp/assets/textures/Checkerboard.png");
    m_Renderer2D->AddQuad({ 0.0f,  0.2f, 0.0f }, { 0.2f, 0.2f }, FL_BLUE);
    m_Renderer2D->AddQuad({ 0.0f, -0.2f, 0.0f }, { 0.2f, 0.2f }, FL_YELLOW);
    m_Renderer2D->AddQuad({ -0.2f, 0.0f, 0.0f }, { 0.2f, 0.2f }, FL_PURPLE);
    m_Renderer2D->End();

    m_Framebuffer->Unbind();
}

void FlameEditorApp::OnUIRender()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::Begin("Viewport");
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

    uint64_t textureID = m_Framebuffer->GetColorAttachmentId();
    ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::Begin("Settings");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Last Render Time: %.3fms", m_LastRenderTime * 0.001f * 0.001f);
    ImGui::End();
}

std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
{
    return std::make_shared<FlameEditorApp>();
}