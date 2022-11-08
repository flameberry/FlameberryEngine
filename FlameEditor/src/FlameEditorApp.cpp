#include "FlameEditorApp.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

FlameEditorApp::FlameEditorApp()
    : m_Framebuffer(Flameberry::OpenGLFramebuffer::Create()),
    m_ViewportSize(1280, 720),
    m_Camera({ 1280.0f, 720.0f }, 1.0f),
    m_Renderer3D(Flameberry::OpenGLRenderer3D::Create())
{
    // Dealing with 3D Renderer
    m_Renderer3D->Init(Flameberry::Application::Get().GetWindow().GetGLFWwindow());

    Flameberry::PerspectiveCameraInfo cameraInfo{};
    cameraInfo.aspectRatio = 1280.0f / 720.0f;
    cameraInfo.FOV = 45.0f;
    cameraInfo.cameraPostion = glm::vec3(0, 0, 2);
    cameraInfo.cameraDirection = glm::vec3(0, 0, -1);
    cameraInfo.zFar = 1000.0f;
    cameraInfo.zNear = 0.1f;

    m_PerspectiveCamera = Flameberry::PerspectiveCamera(cameraInfo);
}

FlameEditorApp::~FlameEditorApp()
{
    m_Renderer3D->CleanUp();
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
        m_Framebuffer->Invalidate();
    }

    m_Framebuffer->Bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    constexpr glm::vec3 clearColor(20.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);

    m_Framebuffer->ClearEntityIDAttachment();

    m_PerspectiveCamera.OnResize(m_ViewportSize.x / m_ViewportSize.y);
    m_PerspectiveCamera.OnUpdate(delta);

    m_Renderer3D->Begin(m_PerspectiveCamera);
    m_Renderer3D->OnDraw();
    m_Renderer3D->End();

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        auto [mx, my] = ImGui::GetMousePos();
        mx -= m_ViewportBounds[0].x;
        my -= m_ViewportBounds[0].y;
        glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
        my = viewportSize.y - my;
        int mouseX = (int)mx;
        int mouseY = (int)my;

        if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
        {
            int entityID = m_Framebuffer->ReadPixel(GL_COLOR_ATTACHMENT1, mouseX, mouseY);
            // FL_LOG("EntityID: {0}", entityID);
            // if (entityID != -1)
            //     m_SceneHierarchyPanel.SetSelectedEntity(m_Scene->GetRegistry()->GetEntityVector()[entityID]);
            // else
            //     m_SceneHierarchyPanel.SetSelectedEntity(Flameberry::entity_handle{ UINT32_MAX, false });
        }
    }

    m_Framebuffer->Unbind();
}

void FlameEditorApp::OnUIRender()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::Begin("Viewport");

    ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
    ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
    ImVec2 viewportOffset = ImGui::GetWindowPos();
    m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
    m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

    uint64_t textureID = m_Framebuffer->GetColorAttachmentId();
    ImGui::Image(reinterpret_cast<ImTextureID>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::Begin("Stats");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Last Render Time: %.3fms", m_LastRenderTime * 0.001f * 0.001f);
    ImGui::End();
}

std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
{
    return std::make_shared<FlameEditorApp>();
}
