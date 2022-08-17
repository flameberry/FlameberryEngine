#include "FlameEditorApp.h"

FlameEditorApp::FlameEditorApp()
    : m_Framebuffer(Flameberry::Framebuffer::Create()), m_ViewportSize(1280, 720), m_Camera(1280.0f / 720.0f, 1.0f)
{
}

FlameEditorApp::~FlameEditorApp()
{
}

void FlameEditorApp::OnRender()
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
    Flameberry::Renderer2D::SetCustomViewportSize(m_ViewportSize);

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    m_Camera.SetAspectRatio(m_ViewportSize.x / m_ViewportSize.y);

    Flameberry::Renderer2D::Begin(m_Camera);
    Flameberry::Renderer2D::AddQuad({ 0, 0, 0 }, { 100, 100 }, FL_PINK);
    Flameberry::Renderer2D::AddQuad({ 100, 0, 0 }, { 100, 100 }, FL_PINK, FL_PROJECT_DIR"SandboxApp/assets/textures/Checkerboard.png");
    Flameberry::Renderer2D::AddQuad({ 0, 100, 0 }, { 100, 100 }, FL_BLUE);
    Flameberry::Renderer2D::AddQuad({ 0, -100, 0 }, { 100, 100 }, { 1.0f, 0.0f, 0.0f, 1.0f });
    Flameberry::Renderer2D::End();

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

std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClient()
{
    return std::make_shared<FlameEditorApp>();
}