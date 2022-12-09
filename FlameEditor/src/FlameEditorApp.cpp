#include "FlameEditorApp.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "project.h"

#include "Utils.h"

namespace Flameberry {
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
        m_TempMesh.Name = "Sphere";

        auto [v, i] = Flameberry::ModelLoader::LoadOBJ(FL_PROJECT_DIR"SandboxApp/assets/models/sponza.obj");
        m_SponzaMesh = Flameberry::Mesh{ v, i };
        m_SponzaMesh.Name = "Sponza";

        auto [v1, i1] = Flameberry::ModelLoader::LoadOBJ(FL_PROJECT_DIR"SandboxApp/assets/models/cylinder.obj");
        Flameberry::Mesh mesh{ v1, i1 };
        mesh.Name = "Cylinder";

        // m_TempMesh.TextureIDs.push_back(Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png"));
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

        // Test
        // std::vector<Flameberry::Mesh> meshes;
        // Flameberry::ModelLoader::LoadOBJ(FL_PROJECT_DIR"SandboxApp/assets/models/sponza.obj", &meshes);
        // FL_LOG("Number of meshes loaded: {0}", meshes.size());
        // meshes.back().TextureIDs.push_back(Flameberry::OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png"));
        // ----

        m_PointLights.emplace_back();
        m_PointLights.back().Position = glm::vec3(-1, 1, 1);
        m_PointLights.back().Color = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f);
        m_PointLights.back().Intensity = 2.0f;

        m_PointLights.emplace_back();
        m_PointLights.back().Position = glm::vec3(0.5f, 0.5f, 0.5f);
        m_PointLights.back().Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        m_PointLights.back().Intensity = 3.0f;

        m_Registry = std::make_shared<Flameberry::Registry>();

        m_SquareEntity = m_Registry->CreateEntity();
        m_Registry->AddComponent<Flameberry::IDComponent>(m_SquareEntity);
        m_Registry->AddComponent<Flameberry::TagComponent>(m_SquareEntity)->Tag = "Sphere";
        m_Registry->AddComponent<Flameberry::TransformComponent>(m_SquareEntity);
        auto meshComponent = m_Registry->AddComponent<Flameberry::MeshComponent>(m_SquareEntity);
        meshComponent->MeshIndex = 0;

        m_BlueSquareEntity = m_Registry->CreateEntity();
        m_Registry->AddComponent<Flameberry::IDComponent>(m_BlueSquareEntity);
        m_Registry->AddComponent<Flameberry::TagComponent>(m_BlueSquareEntity)->Tag = "Sponza";
        m_Registry->AddComponent<Flameberry::TransformComponent>(m_BlueSquareEntity);
        auto meshComponent1 = m_Registry->AddComponent<Flameberry::MeshComponent>(m_BlueSquareEntity);
        meshComponent1->MeshIndex = 1;

        m_ActiveScene = std::make_shared<Flameberry::Scene>(m_Registry.get());
        m_SceneHierarchyPanel = SceneHierarchyPanel(m_ActiveScene.get());
        m_ContentBrowserPanel = ContentBrowserPanel();

        // int inc = 0;
        // for (const auto& mesh : meshes)
        // {
        //     Flameberry::entity_handle entity = m_Registry->CreateEntity();
        //     m_Registry->AddComponent<Flameberry::TransformComponent>(entity);
        //     m_Registry->AddComponent<Flameberry::TagComponent>(entity)->Tag = mesh.Name;
        //     auto meshComponent = m_Registry->AddComponent<Flameberry::MeshComponent>(entity);
        //     meshComponent->MeshIndex = inc;

        //     m_ActiveScene->LoadMesh(mesh);
        //     inc++;
        // }

        m_ActiveScene->LoadMesh(m_TempMesh);
        m_ActiveScene->LoadMesh(m_SponzaMesh);
        m_ActiveScene->LoadMesh(mesh);

        Material metal(glm::vec3(1, 0, 1), 0.2f, true);
        Material nonMetal(glm::vec3(1, 1, 0), 0.7f, false);

        m_ActiveScene->AddMaterial("METAL", metal);
        m_ActiveScene->AddMaterial("YELLOW_NON_METAL", nonMetal);

        m_DirectionalLight.Direction = { -1.0f, -1.0f, -1.0f };
        m_ActiveScene->SetDirectionalLight(m_DirectionalLight);
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

        m_ActiveScene->RenderScene(m_Renderer3D.get(), m_PerspectiveCamera, m_PointLights);

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
                    m_SceneHierarchyPanel.SetSelectedEntity(m_ActiveScene->GetRegistry()->GetEntityVector()[entityID]);
                else
                    m_SceneHierarchyPanel.SetSelectedEntity(Flameberry::entity_handle{ UINT32_MAX, false });
            }
        }
        m_Framebuffer->Unbind();
    }

    void FlameEditorApp::OnUIRender()
    {
        // Main Menu
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save", "Cmd+S"))
                    SaveScene();
                if (ImGui::MenuItem("Open", "Cmd+O"))
                    OpenScene();
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

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

        // Scene File Drop Target
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
            {
                std::string path = (const char*)payload->Data;
                std::filesystem::path scenePath{ path };
                scenePath = project::g_AssetDirectory / scenePath;
                const std::string& ext = scenePath.extension().string();

                FL_LOG("Payload recieved: {0}, with extension {1}", path, ext);

                if (std::filesystem::exists(scenePath) && std::filesystem::is_regular_file(scenePath) && (ext == ".scene" || ext == ".json" || ext == ".berry"))
                    OpenScene(scenePath.string());
                else
                    FL_WARN("Bad File given as Scene!");
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Begin("Stats");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Last Render Time: %.3fms", m_LastRenderTime * 0.001f * 0.001f);

        ImGui::Separator();

        ImGui::Text("Directional Light");
        Utils::DrawVec3Control("Directional", m_DirectionalLight.Direction, 0.0f, 0.01f);
        ImGui::Spacing();
        ImGui::ColorEdit4("Color", glm::value_ptr(m_DirectionalLight.Color));
        ImGui::DragFloat("Intensity", &m_DirectionalLight.Intensity, 0.01f);

        m_ActiveScene->SetDirectionalLight(m_DirectionalLight);

        ImGui::Separator();
        ImGui::Text("Point Lights");
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

        // ImGui::ShowDemoWindow();
    }

    void FlameEditorApp::SaveScene()
    {
        std::string savePath = Flameberry::FileDialog::SaveDialog();
        if (savePath != "")
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.SerializeScene(savePath.c_str());
            FL_LOG("Scene saved to path: {0}", savePath);
            return;
        }
        FL_ERROR("Failed to save scene!");
    }

    void FlameEditorApp::OpenScene()
    {
        std::string sceneToBeLoaded = Flameberry::FileDialog::OpenDialog();
        if (sceneToBeLoaded != "")
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.DeserializeScene(sceneToBeLoaded.c_str());
            FL_INFO("Loaded Scene: {0}", sceneToBeLoaded);
            return;
        }
        FL_ERROR("Failed to load scene!");
    }

    void FlameEditorApp::SaveScene(const std::string& path)
    {
        if (!path.empty())
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.SerializeScene(path.c_str());
        }
    }

    void FlameEditorApp::OpenScene(const std::string& path)
    {
        if (!path.empty())
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.DeserializeScene(path.c_str());
        }
    }

    std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
    {
        return std::make_shared<FlameEditorApp>();
    }
}
