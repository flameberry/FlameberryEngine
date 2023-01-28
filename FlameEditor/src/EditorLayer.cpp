#include "EditorLayer.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "project.h"

#include "Utils.h"

#include "ImGuizmo/ImGuizmo.h"

#define SHADOW_MAP_DIM 4096

namespace Flameberry {
    EditorLayer::EditorLayer()
        : m_ViewportSize(1280, 720),
        m_ShadowMapUniformBuffer(sizeof(glm::mat4), nullptr, GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW)
    {
    }

    EditorLayer::~EditorLayer()
    {
    }

    void EditorLayer::OnCreate()
    {
        OpenGLRenderCommand::EnableBlend();
        OpenGLRenderCommand::EnableDepthTest();
        glEnable(GL_CULL_FACE);

        OpenGLFramebufferSpecification intermediateFramebufferSpec{};
        intermediateFramebufferSpec.FramebufferSize = m_ViewportSize;

        OpenGLFramebufferAttachment colAttach{};
        colAttach.Target = GL_TEXTURE_2D;
        colAttach.InternalFormat = GL_RGBA8;
        colAttach.Format = GL_RGBA;
        colAttach.Type = GL_UNSIGNED_BYTE;
        colAttach.Attachment = GL_COLOR_ATTACHMENT0;
        colAttach.IsColorAttachment = true;
        colAttach.SetupTextureProperties = []() {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        };

        intermediateFramebufferSpec.Attachments = { colAttach };

        m_IntermediateFramebuffer = OpenGLFramebuffer::Create(intermediateFramebufferSpec);

        OpenGLFramebufferSpecification framebufferSpec{};
        framebufferSpec.FramebufferSize = m_ViewportSize;

        OpenGLFramebufferAttachment colorAttachment{};
        colorAttachment.Target = GL_TEXTURE_2D_MULTISAMPLE;
        colorAttachment.InternalFormat = GL_RGBA8;
        colorAttachment.Format = GL_RGBA;
        colorAttachment.Type = GL_UNSIGNED_BYTE;
        colorAttachment.Attachment = GL_COLOR_ATTACHMENT0;
        colorAttachment.IsColorAttachment = true;
        colorAttachment.SetupTextureProperties = []() {
            glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        };

        OpenGLFramebufferAttachment depthAttachment{};
        depthAttachment.Target = GL_TEXTURE_2D_MULTISAMPLE;
        depthAttachment.InternalFormat = GL_DEPTH24_STENCIL8;
        depthAttachment.Format = GL_DEPTH_STENCIL;
        depthAttachment.Type = GL_UNSIGNED_INT_24_8;
        depthAttachment.Attachment = GL_DEPTH_STENCIL_ATTACHMENT;
        depthAttachment.IsColorAttachment = false;
        depthAttachment.SetupTextureProperties = []() {
            glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        };

        framebufferSpec.Attachments = { colorAttachment, depthAttachment };

        m_Framebuffer = OpenGLFramebuffer::Create(framebufferSpec);

        OpenGLFramebufferAttachment mousePickingAttachment{};
        mousePickingAttachment.InternalFormat = GL_R32I;
        mousePickingAttachment.Format = GL_RED_INTEGER;
        mousePickingAttachment.Type = GL_UNSIGNED_BYTE;
        mousePickingAttachment.Attachment = GL_COLOR_ATTACHMENT0;
        mousePickingAttachment.IsColorAttachment = true;
        mousePickingAttachment.SetupTextureProperties = []() {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        };

        OpenGLFramebufferAttachment mousePickingDepthAttachment{};
        mousePickingDepthAttachment.InternalFormat = GL_DEPTH_COMPONENT;
        mousePickingDepthAttachment.Format = GL_DEPTH_COMPONENT;
        mousePickingDepthAttachment.Type = GL_FLOAT;
        mousePickingDepthAttachment.Attachment = GL_DEPTH_ATTACHMENT;
        mousePickingDepthAttachment.IsColorAttachment = false;
        mousePickingDepthAttachment.SetupTextureProperties = []() {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        };

        OpenGLFramebufferSpecification mousePickingFramebufferSpec{};
        mousePickingFramebufferSpec.FramebufferSize = m_ViewportSize;
        mousePickingFramebufferSpec.Attachments = { mousePickingAttachment, mousePickingDepthAttachment };

        m_MousePickingFramebuffer = OpenGLFramebuffer::Create(mousePickingFramebufferSpec);

        OpenGLShaderBinding cameraBinding;
        cameraBinding.blockName = "Camera";
        cameraBinding.blockBindingIndex = 0;

        m_MousePickingShader = OpenGLShader::Create(FL_PROJECT_DIR"Flameberry/assets/shaders/opengl/mouse_picking.glsl", { cameraBinding });

        OpenGLFramebufferAttachment shadowMapFramebufferDepthAttachment{};
        shadowMapFramebufferDepthAttachment.InternalFormat = GL_DEPTH_COMPONENT;
        shadowMapFramebufferDepthAttachment.Format = GL_DEPTH_COMPONENT;
        shadowMapFramebufferDepthAttachment.Type = GL_FLOAT;
        shadowMapFramebufferDepthAttachment.Attachment = GL_DEPTH_ATTACHMENT;
        shadowMapFramebufferDepthAttachment.IsColorAttachment = false;
        shadowMapFramebufferDepthAttachment.SetupTextureProperties = []() {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        };

        OpenGLFramebufferSpecification shadowMapFramebufferSpec;
        shadowMapFramebufferSpec.FramebufferSize = { SHADOW_MAP_DIM, SHADOW_MAP_DIM };
        shadowMapFramebufferSpec.Attachments = { shadowMapFramebufferDepthAttachment };

        m_ShadowMapFramebuffer = OpenGLFramebuffer::Create(shadowMapFramebufferSpec);

        OpenGLShaderBinding binding{};
        binding.blockBindingIndex = 3;
        binding.blockName = "Camera";

        m_ShadowMapShader = OpenGLShader::Create(FL_PROJECT_DIR"Flameberry/assets/shaders/opengl/shadow_map.glsl", { binding });
        m_ShadowMapUniformBuffer.BindBufferBase(binding.blockBindingIndex);

        PerspectiveCameraInfo cameraInfo{};
        cameraInfo.aspectRatio = 1280.0f / 720.0f;
        cameraInfo.FOV = 45.0f;
        cameraInfo.cameraPostion = glm::vec3(0, 0, 2);
        cameraInfo.cameraDirection = glm::vec3(0, 0, -1);
        cameraInfo.zFar = 1000.0f;
        cameraInfo.zNear = 0.1f;

        m_EditorCamera = PerspectiveCamera(cameraInfo);

        m_TempMesh = Mesh(FL_PROJECT_DIR"SandboxApp/assets/models/sphere.obj");
        m_TempMesh.Name = "Sphere";

        m_SponzaMesh = Mesh(FL_PROJECT_DIR"SandboxApp/assets/models/platform.obj");
        m_SponzaMesh.Name = "Sponza";

        Mesh mesh(FL_PROJECT_DIR"SandboxApp/assets/models/cylinder.obj");
        mesh.Name = "Cylinder";

        m_BrickTexture = OpenGLTexture::Create(FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png");

        // Test
        // std::vector<Mesh> meshes;
        // ModelLoader::LoadOBJ(FL_PROJECT_DIR"SandboxApp/assets/models/sponza.obj", &meshes);
        // FL_LOG("Number of meshes loaded: {0}", meshes.size());
        // meshes.back().TextureIDs.push_back(OpenGLRenderCommand::CreateTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png"));
        // ----

        m_Registry = std::make_shared<ecs::registry>();

        m_SquareEntity = m_Registry->create();
        m_Registry->emplace<IDComponent>(m_SquareEntity);
        m_Registry->emplace<TagComponent>(m_SquareEntity).Tag = "Sphere";
        m_Registry->emplace<TransformComponent>(m_SquareEntity);
        auto& meshComponent = m_Registry->emplace<MeshComponent>(m_SquareEntity);
        meshComponent.MeshIndex = 0;

        m_BlueSquareEntity = m_Registry->create();
        m_Registry->emplace<IDComponent>(m_BlueSquareEntity);
        m_Registry->emplace<TagComponent>(m_BlueSquareEntity).Tag = "Sponza";
        m_Registry->emplace<TransformComponent>(m_BlueSquareEntity);
        auto& meshComponent1 = m_Registry->emplace<MeshComponent>(m_BlueSquareEntity);
        meshComponent1.MeshIndex = 1;

        ecs::entity_handle entity = m_Registry->create();
        m_Registry->emplace<IDComponent>(entity);
        m_Registry->emplace<TagComponent>(entity).Tag = "Light";
        m_Registry->emplace<TransformComponent>(entity).translation = glm::vec3(1.0f);
        auto& light = m_Registry->emplace<LightComponent>(entity);
        light.Color = glm::vec3(1.0f);
        light.Intensity = 2.0f;

        m_ActiveScene = std::make_shared<Scene>(m_Registry.get());
        m_SceneHierarchyPanel = SceneHierarchyPanel(m_ActiveScene.get());
        m_ContentBrowserPanel = ContentBrowserPanel();

        // int inc = 0;
        // for (const auto& mesh : meshes)
        // {
        //     entity_handle entity = m_Registry->create();
        //     m_Registry->AddComponent<TransformComponent>(entity);
        //     m_Registry->AddComponent<TagComponent>(entity)->Tag = mesh.Name;
        //     auto meshComponent = m_Registry->AddComponent<MeshComponent>(entity);
        //     meshComponent->MeshIndex = inc;

        //     m_ActiveScene->LoadMesh(mesh);
        //     inc++;
        // }

        m_ActiveScene->LoadMesh(m_TempMesh);
        m_ActiveScene->LoadMesh(m_SponzaMesh);
        m_ActiveScene->LoadMesh(mesh);

        Material metal(glm::vec3(1, 0, 1), 0.2f, true);
        Material nonMetal(glm::vec3(1, 1, 0), 0.7f, false);
        nonMetal.TextureMap = m_BrickTexture;

        m_ActiveScene->AddMaterial("METAL", metal);
        m_ActiveScene->AddMaterial("YELLOW_NON_METAL", nonMetal);

        m_DirectionalLight.Direction = { -1.0f, -1.0f, -1.0f };
        m_DirectionalLight.Color = glm::vec3(1.0f);
        m_DirectionalLight.Intensity = 2.0f;
        m_ActiveScene->SetDirectionalLight(m_DirectionalLight);

        m_SceneRenderer = SceneRenderer::Create();
    }

    void EditorLayer::OnUpdate(float delta)
    {
        FL_PROFILE_SCOPE("Last Frame Render");

        glm::mat4 lightViewProjectionMatrix(1.0f);

        {
            FL_PROFILE_SCOPE("Shadow Pass");
            // glCullFace(GL_FRONT);
            m_ShadowMapFramebuffer->Bind();
            OpenGLRenderCommand::SetViewport(0, 0, SHADOW_MAP_DIM, SHADOW_MAP_DIM);
            glClear(GL_DEPTH_BUFFER_BIT);

            // Render
            glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 10.0f);
            glm::mat4 lightView = glm::lookAt(
                // glm::vec3(1.0f),
                -m_DirectionalLight.Direction,
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            lightViewProjectionMatrix = lightProjection * lightView;

            m_ShadowMapUniformBuffer.Bind();
            m_ShadowMapUniformBuffer.BufferSubData(glm::value_ptr(lightViewProjectionMatrix), sizeof(glm::mat4), 0);

            m_SceneRenderer->RenderSceneForShadowMapping(m_ActiveScene, m_ShadowMapShader);

            m_ShadowMapUniformBuffer.Unbind();
            m_ShadowMapFramebuffer->Unbind();
            // glCullFace(GL_BACK);
        }

        // Framebuffer Resize
        if (glm::vec2 framebufferSize = m_Framebuffer->GetFramebufferSize();
            m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
            (framebufferSize.x != m_ViewportSize.x || framebufferSize.y != m_ViewportSize.y))
        {
            m_Framebuffer->SetFramebufferSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_Framebuffer->Invalidate();

            m_IntermediateFramebuffer->SetFramebufferSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_IntermediateFramebuffer->Invalidate();

            m_MousePickingFramebuffer->SetFramebufferSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_MousePickingFramebuffer->Invalidate();
        }

        {
            FL_PROFILE_SCOPE("Render Pass");
            m_Framebuffer->Bind();
            OpenGLRenderCommand::SetViewport(0, 0, m_ViewportSize.x, m_ViewportSize.y);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // constexpr glm::vec3 clearColor(20.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f);
            const auto& clearColor = m_ActiveScene->GetClearColor();
            // glm::vec3 clearColor(0.0f);
            glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);

            m_EditorCamera.OnResize(m_ViewportSize.x / m_ViewportSize.y);

            if (m_IsViewportFocused)
                m_IsCameraMoving = m_EditorCamera.OnUpdate(delta);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_ShadowMapFramebuffer->GetColorAttachmentID());
            m_SceneRenderer->RenderScene(m_ActiveScene, m_EditorCamera, lightViewProjectionMatrix);

            // Copy
            m_Framebuffer->SetRead();
            m_IntermediateFramebuffer->SetWrite();
            glBlitFramebuffer(0, 0, m_ViewportSize.x, m_ViewportSize.y, 0, 0, m_ViewportSize.x, m_ViewportSize.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

            m_Framebuffer->Unbind();
        }

        {
            FL_PROFILE_SCOPE("Mouse Picking Pass");

            m_MousePickingFramebuffer->Bind();
            glClear(GL_DEPTH_BUFFER_BIT);

            int clearValue = -1;
            glClearBufferiv(GL_COLOR, 0, &clearValue);

            m_SceneRenderer->RenderSceneForMousePicking(m_ActiveScene, m_MousePickingShader);

            bool attemptedToSelect = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_IsGizmoActive;
            // bool attemptedToMoveCamera = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
            if (attemptedToSelect)
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
                    int entityID = m_Framebuffer->ReadPixel(GL_COLOR_ATTACHMENT0, mouseX, mouseY);
                    m_SceneHierarchyPanel.SetSelectedEntity((entityID != -1) ? ecs::entity_handle(entityID) : ecs::entity_handle::null);
                }
            }
            m_MousePickingFramebuffer->Unbind();
        }
    }

    void EditorLayer::OnUIRender()
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

        m_IsViewportFocused = ImGui::IsWindowFocused();

        ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportOffset = ImGui::GetWindowPos();
        m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
        m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

        uint64_t textureID = m_IntermediateFramebuffer->GetColorAttachmentID();
        ImGui::Image(reinterpret_cast<ImTextureID>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

        // ImGuizmo
        const auto& selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity != ecs::entity_handle::null && m_GizmoType != -1)
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            float windowWidth = (float)ImGui::GetWindowWidth();
            float windowHeight = (float)ImGui::GetWindowHeight();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

            const auto& projectionMatrix = m_EditorCamera.GetProjectionMatrix();
            const auto& viewMatrix = m_EditorCamera.GetViewMatrix();

            auto& transformComp = m_Registry->get<TransformComponent>(selectedEntity);
            glm::mat4 transform = transformComp.GetTransform();

            bool snap = Input::IsKey(GLFW_KEY_LEFT_CONTROL, GLFW_PRESS);
            float snapValue = 0.5f;
            if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
                snapValue = 45.0f;

            float snapValues[3] = { snapValue, snapValue, snapValue };

            ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projectionMatrix), (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform), nullptr, snap ? snapValues : nullptr);
            m_IsGizmoActive = ImGuizmo::IsUsing();
            if (m_IsGizmoActive)
            {
                m_IsGizmoActive = true;
                glm::vec3 translation, rotation, scale;
                DecomposeTransform(transform, translation, rotation, scale);

                transformComp.translation = translation;

                glm::vec3 deltaRotation = rotation - transformComp.rotation;
                transformComp.rotation += deltaRotation;
                transformComp.scale = scale;
            }
        }

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

        FL_DISPLAY_SCOPE_DETAILS_IMGUI();

        ImGui::Separator();
        if (ImGui::Button("Reload Mesh Shader"))
            m_SceneRenderer->ReloadShader();
        ImGui::End();

        m_SceneHierarchyPanel.OnUIRender();
        m_ContentBrowserPanel.OnUIRender();
    }

    void EditorLayer::OnEvent(Event& e)
    {
        switch (e.GetType())
        {
        case EventType::MOUSEBUTTON_PRESSED:
            this->OnMouseButtonPressedEvent(*(MouseButtonPressedEvent*)(&e));
            break;
        case EventType::KEY_PRESSED:
            this->OnKeyPressedEvent(*(KeyPressedEvent*)(&e));
            break;
        case EventType::NONE:
            break;
        }
    }

    void EditorLayer::OnKeyPressedEvent(KeyPressedEvent& e)
    {
        bool ctrl_or_cmd = Input::IsKey(GLFW_KEY_LEFT_SUPER, GLFW_PRESS);
        bool shift = Input::IsKey(GLFW_KEY_LEFT_SHIFT, GLFW_PRESS) || Input::IsKey(GLFW_KEY_RIGHT_SHIFT, GLFW_PRESS);
        switch (e.KeyCode)
        {
        case GLFW_KEY_O:
            if (ctrl_or_cmd)
                OpenScene();
            break;
        case GLFW_KEY_S:
            if (ctrl_or_cmd && shift)
                SaveScene();
            break;
        case GLFW_KEY_Q:
            if (!m_IsCameraMoving && !m_IsGizmoActive && m_IsViewportFocused)
                m_GizmoType = -1;
            break;
        case GLFW_KEY_W:
            if (!m_IsCameraMoving && !m_IsGizmoActive && m_IsViewportFocused)
                m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
            break;
        case GLFW_KEY_E:
            if (!m_IsCameraMoving && !m_IsGizmoActive && m_IsViewportFocused)
                m_GizmoType = ImGuizmo::OPERATION::ROTATE;
            break;
        case GLFW_KEY_R:
            if (!m_IsCameraMoving && !m_IsGizmoActive && m_IsViewportFocused)
                m_GizmoType = ImGuizmo::OPERATION::SCALE;
            break;
        }
    }

    void EditorLayer::OnMouseButtonPressedEvent(MouseButtonPressedEvent& e)
    {
        switch (e.KeyCode)
        {
        case GLFW_MOUSE_BUTTON_LEFT:
            break;
        }
    }

    void EditorLayer::SaveScene()
    {
        std::string savePath = FileDialog::SaveDialog();
        if (savePath != "")
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.SerializeScene(savePath.c_str());
            FL_LOG("Scene saved to path: {0}", savePath);
            return;
        }
        FL_ERROR("Failed to save scene!");
    }

    void EditorLayer::OpenScene()
    {
        std::string sceneToBeLoaded = FileDialog::OpenDialog();
        if (sceneToBeLoaded != "")
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.DeserializeScene(sceneToBeLoaded.c_str());
            FL_INFO("Loaded Scene: {0}", sceneToBeLoaded);
            return;
        }
        FL_ERROR("Failed to load scene!");
    }

    void EditorLayer::SaveScene(const std::string& path)
    {
        if (!path.empty())
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.SerializeScene(path.c_str());
        }
    }

    void EditorLayer::OpenScene(const std::string& path)
    {
        if (!path.empty())
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.DeserializeScene(path.c_str());
        }
    }

    void EditorLayer::OnDestroy()
    {
    }
}
