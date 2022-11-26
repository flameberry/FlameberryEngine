#include "FlameEditorApp.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

#include "Core/UUID.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Utils.h"

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

    auto [vertices, alt_indices] = Flameberry::ModelLoader::LoadOBJ(FL_PROJECT_DIR"SandboxApp/assets/models/sphere.obj");
    m_TempMesh = Flameberry::Mesh{ vertices, alt_indices };
    auto [v, i] = Flameberry::ModelLoader::LoadOBJ(FL_PROJECT_DIR"SandboxApp/assets/models/sponza.obj");
    m_SponzaMesh = Flameberry::Mesh{ v, i };

    auto [v1, i1] = Flameberry::ModelLoader::LoadOBJ(FL_PROJECT_DIR"SandboxApp/assets/models/cylinder.obj");
    Flameberry::Mesh mesh{ v1, i1 };

    m_TempMesh.TextureIDs.push_back(Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png"));
    m_SponzaMesh.TextureIDs.push_back(Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png"));

    m_FloorMesh.Vertices.emplace_back();
    m_FloorMesh.Vertices.back().position = { -1.0f, 0.0f, 1.0f };
    m_FloorMesh.Vertices.back().texture_uv = { 0.0f, 0.0f };
    m_FloorMesh.Vertices.back().texture_index = 0.0f;
    m_FloorMesh.Vertices.back().normal = { 0.0f, 1.0f, 0.0f };
    m_FloorMesh.Vertices.back().entityID = 0.0f;

    m_FloorMesh.Vertices.emplace_back();
    m_FloorMesh.Vertices.back().position = { -1.0f, 0.0f, -1.0f };
    m_FloorMesh.Vertices.back().texture_uv = { 0.0f, 1.0f };
    m_FloorMesh.Vertices.back().texture_index = 0.0f;
    m_FloorMesh.Vertices.back().normal = { 0.0f, 1.0f, 0.0f };
    m_FloorMesh.Vertices.back().entityID = 0.0f;

    m_FloorMesh.Vertices.emplace_back();
    m_FloorMesh.Vertices.back().position = { 1.0f, 0.0f, -1.0f };
    m_FloorMesh.Vertices.back().texture_uv = { 1.0f, 1.0f };
    m_FloorMesh.Vertices.back().texture_index = 0.0f;
    m_FloorMesh.Vertices.back().normal = { 0.0f, 1.0f, 0.0f };
    m_FloorMesh.Vertices.back().entityID = 0.0f;

    m_FloorMesh.Vertices.emplace_back();
    m_FloorMesh.Vertices.back().position = { 1.0f, 0.0f, 1.0f };
    m_FloorMesh.Vertices.back().texture_uv = { 1.0f, 0.0f };
    m_FloorMesh.Vertices.back().texture_index = 0.0f;
    m_FloorMesh.Vertices.back().normal = { 0.0f, 1.0f, 0.0f };
    m_FloorMesh.Vertices.back().entityID = 0.0f;

    m_FloorMesh.Indices = {
        0, 1, 2,
        0, 2, 3
    };

    m_FloorMesh.TextureIDs.push_back(Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/planks.png"));
    m_FloorMesh.TextureIDs.push_back(Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/planksSpec.png"));
    m_FloorMesh.Invalidate();

    m_Meshes.push_back(m_TempMesh);
    m_Meshes.push_back(m_SponzaMesh);
    m_Meshes.push_back(mesh);
    // m_Meshes.push_back(m_FloorMesh);

    // m_PointLights.push_back(PointLight(glm::vec3(1.0f), glm::vec4(1.0f), 5.0f));

    // Set point light data
    // m_PointLights.emplace_back();
    // m_PointLights.back().Position = glm::vec3(0.4, 1, 1);
    // m_PointLights.back().Color = glm::vec4(0, 0, 1, 1);
    // m_PointLights.back().Intensity = 2.0f;

    // m_PointLights.emplace_back();
    // m_PointLights.back().Position = glm::vec3(0, 0, 1.5);
    // m_PointLights.back().Color = glm::vec4(1, 1, 0, 1);
    // m_PointLights.back().Intensity = 2.0f;

    // m_PointLights.emplace_back();
    // m_PointLights.back().Position = glm::vec3(0, 0, 4);
    // m_PointLights.back().Color = glm::vec4(1);
    // m_PointLights.back().Intensity = 8.0f;

    m_PointLights.emplace_back();
    m_PointLights.back().Position = glm::vec3(-1, 1, 1);
    m_PointLights.back().Color = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f);
    m_PointLights.back().Intensity = 2.0f;

    m_PointLights.emplace_back();
    m_PointLights.back().Position = glm::vec3(0.5f, 0.5f, 0.5f);
    m_PointLights.back().Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_PointLights.back().Intensity = 3.0f;

    // m_PointLights.emplace_back();
    // m_PointLights.back().Position = glm::vec3(0, 10, 1);
    // m_PointLights.back().Color = glm::vec4(1, 0, 1, 1);
    // m_PointLights.back().Intensity = 40.0f;

    // m_PointLights.emplace_back();
    // m_PointLights.back().Position = glm::vec3(10, 10, 1);
    // m_PointLights.back().Color = glm::vec4(1, 0.5, 0, 1);
    // m_PointLights.back().Intensity = 40.0f;

    m_Registry = std::make_shared<Flameberry::Registry>();
    m_SquareEntity = m_Registry->CreateEntity();
    m_Registry->AddComponent<Flameberry::TransformComponent>(m_SquareEntity);
    m_Registry->AddComponent<Flameberry::TagComponent>(m_SquareEntity)->Tag = "Sphere";
    auto meshComponent = m_Registry->AddComponent<Flameberry::MeshComponent>(m_SquareEntity);
    meshComponent->MeshIndex = 0;

    m_BlueSquareEntity = m_Registry->CreateEntity();
    m_Registry->AddComponent<Flameberry::TagComponent>(m_BlueSquareEntity)->Tag = "Sponza";
    m_Registry->AddComponent<Flameberry::TransformComponent>(m_BlueSquareEntity);
    auto meshComponent1 = m_Registry->AddComponent<Flameberry::MeshComponent>(m_BlueSquareEntity);
    meshComponent1->MeshIndex = 1;

    m_Scene = std::make_shared<Flameberry::Scene>(m_Registry.get());
    m_SceneHierarchyPanel = SceneHierarchyPanel(m_Scene.get(), &m_Meshes);
    m_ContentBrowserPanel = ContentBrowserPanel();
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

    // constexpr glm::vec3 clearColor(20.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f);
    glm::vec3 clearColor(0.0f);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);

    m_Framebuffer->ClearEntityIDAttachment();

    m_PerspectiveCamera.OnResize(m_ViewportSize.x / m_ViewportSize.y);
    m_PerspectiveCamera.OnUpdate(delta);

    m_Scene->RenderScene(m_Renderer3D.get(), m_PerspectiveCamera, &m_Meshes, m_PointLights);

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
            if (entityID != -1)
                m_SceneHierarchyPanel.SetSelectedEntity(m_Scene->GetRegistry()->GetEntityVector()[entityID]);
            else
                m_SceneHierarchyPanel.SetSelectedEntity(Flameberry::entity_handle{ UINT32_MAX, false });
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

    ImGui::Separator();

    int i = 0;
    for (auto& light : m_PointLights)
    {
        ImGui::PushID(i);
        ImGui::Text("Light - %d", i);
        Utils::DrawVec3Control("Position", light.Position, 0.0f, 0.01f);
        ImGui::Spacing();
        ImGui::ColorEdit4("Color", glm::value_ptr(light.Color));
        ImGui::DragFloat("Intensity", &light.Intensity, 0.1f);
        ImGui::PopID();
        i++;
    }

    ImGui::End();

    m_SceneHierarchyPanel.OnUIRender();
    m_ContentBrowserPanel.OnUIRender();
}

std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
{
    return std::make_shared<FlameEditorApp>();
}
