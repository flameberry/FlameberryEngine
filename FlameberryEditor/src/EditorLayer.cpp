#include "EditorLayer.h"

#include <fmt/format.h>

#include <imgui.h>
#include <IconFontCppHeaders/IconsLucide.h>

#include "ImGuizmo/ImGuizmo.h"
#include "Renderer/Framebuffer.h"
#include "UI.h"

#include "Physics/PhysicsEngine.h"
#include "Renderer/ShaderLibrary.h"
#include "Renderer/Skymap.h"

namespace Flameberry {

    class MovingActor : public Flameberry::Actor {
    public:
        void OnInstanceCreated() override {
            FBY_LOG("Created MovingActor!");
        }
        void OnInstanceDeleted() override {
            FBY_LOG("Deleted MovingActor!");
        }
        void OnUpdate(float delta) override {
            auto& transform = GetComponent<TransformComponent>();

            float speed = 10.0f;

            if (Input::IsKeyPressed(KeyCode::W))
                transform.Translation.z -= speed * delta;
            if (Input::IsKeyPressed(KeyCode::S))
                transform.Translation.z += speed * delta;
            if (Input::IsKeyPressed(KeyCode::A))
                transform.Translation.x -= speed * delta;
            if (Input::IsKeyPressed(KeyCode::D))
                transform.Translation.x += speed * delta;
        }
    };

    struct CameraUniformBufferObject { glm::mat4 ViewMatrix, ProjectionMatrix, ViewProjectionMatrix; };
    EditorLayer::EditorLayer(const Ref<Project>& project)
        : m_Project(project), m_ActiveCameraController(PerspectiveCameraSpecification{
            glm::vec3(0.0f, 2.0f, 4.0f),
            glm::vec3(0.0f, -0.3f, -1.0f),
            (float)Application::Get().GetWindow().GetSpecification().Width / (float)Application::Get().GetWindow().GetSpecification().Height,
            45.0f,
            0.1f,
            1000.0f
            }
        )
    {
#ifdef FBY_PLATFORM_MACOS
        Platform::SetNewSceneCallbackMenuBar(FBY_BIND_EVENT_FN(EditorLayer::NewScene));
        Platform::SetSaveSceneCallbackMenuBar(FBY_BIND_EVENT_FN(EditorLayer::SaveScene));
        Platform::SetSaveSceneAsCallbackMenuBar(FBY_BIND_EVENT_FN(EditorLayer::SaveSceneAs));
        Platform::SetOpenSceneCallbackMenuBar(FBY_BIND_EVENT_FN(EditorLayer::OpenScene));
        Platform::CreateMenuBar();
#endif
    }

    void EditorLayer::OnCreate()
    {
        m_CursorIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR"FlameberryEditor/Assets/Icons/cursor_icon.png");
        m_TranslateIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR"FlameberryEditor/Assets/Icons/translate_icon_2.png");
        m_RotateIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR"FlameberryEditor/Assets/Icons/rotate_icon.png");
        m_ScaleIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR"FlameberryEditor/Assets/Icons/scale_icon.png");
        m_PlayAndStopIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR"FlameberryEditor/Assets/Icons/PlayAndStopButtonIcon.png");
        m_SettingsIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR"FlameberryEditor/Assets/Icons/SettingsIcon.png");
        m_PauseIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR"FlameberryEditor/Assets/Icons/PauseIcon.png");
        m_StepIcon = Texture2D::TryGetOrLoadTexture(FBY_PROJECT_DIR"FlameberryEditor/Assets/Icons/StepIcon.png");

        Project::SetActive(m_Project);
        std::filesystem::current_path(m_Project->GetProjectDirectory());

        PhysicsEngine::Init();
        m_ActiveScene = CreateRef<Scene>();
        m_SceneHierarchyPanel = CreateRef<SceneHierarchyPanel>(m_ActiveScene);
        m_ContentBrowserPanel = CreateRef<ContentBrowserPanel>();

        // Open the start scene
        if (const auto& scenePath = m_Project->GetStartScenePath(); !scenePath.empty())
        {
            if (std::filesystem::exists(scenePath))
                OpenScene(scenePath);
            else
                FBY_ERROR("Start scene path: {} does not exist", scenePath);
        }

        /////////////////////////////////////// Preparing Mouse Picking Pass ////////////////////////////////////////

        BufferSpecification mousePickingBufferSpec;
        mousePickingBufferSpec.InstanceCount = 1;
        mousePickingBufferSpec.InstanceSize = sizeof(int32_t);
        mousePickingBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        mousePickingBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        m_MousePickingBuffer = std::make_unique<Buffer>(mousePickingBufferSpec);

        FramebufferSpecification mousePickingFramebufferSpec;
        mousePickingFramebufferSpec.Width = m_ViewportSize.x;
        mousePickingFramebufferSpec.Height = m_ViewportSize.y;
        mousePickingFramebufferSpec.Samples = 1;
        mousePickingFramebufferSpec.Attachments = { VK_FORMAT_R32_SINT, VK_FORMAT_D32_SFLOAT };
        mousePickingFramebufferSpec.ClearColorValue = { .int32 = { -1 } };
        mousePickingFramebufferSpec.DepthStencilClearValue = { 1.0f, 0 };

        RenderPassSpecification mousePickingRenderPassSpec;
        mousePickingRenderPassSpec.TargetFramebuffers = { CreateRef<Framebuffer>(mousePickingFramebufferSpec) };

        m_MousePickingRenderPass = CreateRef<RenderPass>(mousePickingRenderPassSpec);

        // Creating Descriptors
        DescriptorSetLayoutSpecification mousePickingDescSetLayoutSpec;
        mousePickingDescSetLayoutSpec.Bindings.emplace_back();
        mousePickingDescSetLayoutSpec.Bindings[0].binding = 0;
        mousePickingDescSetLayoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        mousePickingDescSetLayoutSpec.Bindings[0].descriptorCount = 1;
        mousePickingDescSetLayoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        mousePickingDescSetLayoutSpec.Bindings[0].pImmutableSamplers = nullptr;

        m_MousePickingDescriptorSetLayout = DescriptorSetLayout::CreateOrGetCached(mousePickingDescSetLayoutSpec);

        {
            // Pipeline Creation
            PipelineSpecification pipelineSpec{};
            pipelineSpec.Shader = ShaderLibrary::Get("Flameberry_MousePicking");
            pipelineSpec.RenderPass = m_MousePickingRenderPass;

            pipelineSpec.VertexLayout = {
                ShaderDataType::Float3,  // a_Position
                ShaderDataType::Dummy12, // Normal (Unnecessary)
                ShaderDataType::Dummy8,  // TextureCoords (Unnecessary)
                ShaderDataType::Dummy12, // Tangent (Unnecessary)
                ShaderDataType::Dummy12  // BiTangent (Unnecessary)
            };

            m_MousePickingPipeline = CreateRef<Pipeline>(pipelineSpec);
        }

        {
            // Pipeline Creation
            PipelineSpecification pipelineSpec{};
            pipelineSpec.Shader = ShaderLibrary::Get("Flameberry_MousePicking2D");
            pipelineSpec.RenderPass = m_MousePickingRenderPass;

            pipelineSpec.VertexLayout = {
                ShaderDataType::Float3,  // a_Position
                ShaderDataType::Dummy12, // Color (Unnecessary)
                ShaderDataType::Int      // a_EntityIndex
            };
            pipelineSpec.CullMode = VK_CULL_MODE_FRONT_BIT;

            m_MousePicking2DPipeline = CreateRef<Pipeline>(pipelineSpec);
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

        m_SceneRenderer = std::make_unique<SceneRenderer>(m_RenderViewportSize);

        auto swapchain = VulkanContext::GetCurrentWindow()->GetSwapChain();
        uint32_t imageCount = swapchain->GetSwapChainImageCount();

        m_ViewportDescriptorSets.resize(imageCount);
        m_CompositePassViewportDescriptorSets.resize(imageCount);
        for (int i = 0; i < imageCount; i++)
        {
            m_ViewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                Texture2D::GetDefaultSampler(),
                m_SceneRenderer->GetGeometryPassOutputImageView(i),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

#if 0
            m_CompositePassViewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                Texture2D::GetDefaultSampler(),
                m_SceneRenderer->GetCompositePassOutputImageView(i),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
#endif
        }
    }

    void EditorLayer::OnUpdate(float delta)
    {
        FBY_PROFILE_SCOPE("EditorLayer::OnUpdate");
        if (m_HasViewportSizeChanged)
        {
            m_ActiveCameraController.GetPerspectiveCamera()->OnResize(m_ViewportSize.x / m_ViewportSize.y);
            m_ActiveScene->OnViewportResize(m_ViewportSize);
            m_HasViewportSizeChanged = false;
        }

        if (m_IsCameraMoving || m_IsViewportHovered)
            m_IsCameraMoving = m_ActiveCameraController.OnUpdate(delta);
        Application::Get().BlockAllEvents(m_IsCameraMoving);

        if (m_ShouldReloadMeshShaders)
        {
            VulkanContext::GetCurrentDevice()->WaitIdle();
            m_SceneRenderer->ReloadMeshShaders();
            m_ShouldReloadMeshShaders = false;
        }

        // Updating Scene
        switch (m_EditorState)
        {
            case EditorState::Edit:
            {
                // TODO: Design this better
                auto camera = m_ActiveCameraController.GetPerspectiveCamera();

                // Actual Rendering (All scene related render passes)
                m_SceneRenderer->RenderScene(m_RenderViewportSize, m_ActiveScene, camera->GetViewMatrix(), camera->GetProjectionMatrix(), camera->GetSpecification().Position, camera->GetSpecification().zNear, camera->GetSpecification().zFar, m_SceneHierarchyPanel->GetSelectionContext(), m_EnableGrid);
                break;
            }
            case EditorState::Play:
            {
                m_ActiveScene->OnUpdateRuntime(delta);

                // TODO: Design this better
                const auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
                if (cameraEntity != fbentt::null)
                {
                    auto [transform, cameraComp] = m_ActiveScene->GetRegistry()->get<TransformComponent, CameraComponent>(cameraEntity);
                    auto& camera = cameraComp.Camera;
                    camera.SetView(transform.Translation, transform.Rotation);
                    m_SceneRenderer->RenderScene(m_RenderViewportSize, m_ActiveScene, camera.GetViewMatrix(), camera.GetProjectionMatrix(), transform.Translation, camera.GetSettings().Near, camera.GetSettings().Far, fbentt::null, false, false, false, false);
                }
                else
                {
                    auto camera = m_ActiveCameraController.GetPerspectiveCamera();
                    m_SceneRenderer->RenderScene(m_RenderViewportSize, m_ActiveScene, camera->GetViewMatrix(), camera->GetProjectionMatrix(), camera->GetSpecification().Position, camera->GetSpecification().zNear, camera->GetSpecification().zFar, m_SceneHierarchyPanel->GetSelectionContext(), m_EnableGrid);
                }
                break;
            }
        }

        // Update all image index related descriptors
        Renderer::Submit([&](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                // TODO: Update these descriptors only when there corresponding framebuffer is updated
                InvalidateViewportImGuiDescriptorSet(imageIndex);
#if 0
                InvalidateCompositePassImGuiDescriptorSet(imageIndex);
#endif
            }
        );

        if (m_IsMousePickingBufferReady)
        {
            RenderCommand::WritePixelFromImageToBuffer(
                m_MousePickingBuffer->GetVulkanBuffer(),
                m_MousePickingRenderPass->GetSpecification().TargetFramebuffers[0]->GetColorAttachment(0)->GetImage(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                { m_MouseX, m_ViewportSize.y - m_MouseY - 1 }
            );

            m_MousePickingBuffer->MapMemory(sizeof(int32_t));
            int32_t* data = (int32_t*)m_MousePickingBuffer->GetMappedMemory();
            int32_t entityIndex = data[0];
            m_MousePickingBuffer->UnmapMemory();
            m_SceneHierarchyPanel->SetSelectionContext((entityIndex != -1) ? m_ActiveScene->GetRegistry()->get_entity_at_index(entityIndex) : fbentt::null);
            // FBY_LOG("Selected Entity Index: {}", entityIndex);
            m_IsMousePickingBufferReady = false;
        }

        // bool attemptedToMoveCamera = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
        bool attemptedToSelect = ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && m_DidViewportBegin
            && !m_IsAnyOverlayHovered
            && !m_IsGizmoActive;

        if (attemptedToSelect)
        {
            auto [mx, my] = ImGui::GetMousePos();
            mx -= m_ViewportBounds[0].x;
            my -= m_ViewportBounds[0].y;
            glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
            my = viewportSize.y - my;
            m_MouseX = (int)mx;
            m_MouseY = (int)my;

            if (m_MouseX >= 0 && m_MouseY >= 0 && m_MouseX < (int)viewportSize.x && m_MouseY < (int)viewportSize.y)
            {
                FBY_PROFILE_SCOPE("Last Mouse Picking Pass");
                m_IsMousePickingBufferReady = true;

                auto framebuffer = m_MousePickingRenderPass->GetSpecification().TargetFramebuffers[0];
                if (framebuffer->GetSpecification().Width != m_ViewportSize.x || framebuffer->GetSpecification().Height != m_ViewportSize.y)
                    framebuffer->OnResize(m_ViewportSize.x, m_ViewportSize.y, m_MousePickingRenderPass->GetRenderPass());

                RenderCommand::SetViewport(0, 0, m_ViewportSize.x, m_ViewportSize.y);
                m_SceneRenderer->RenderSceneForMousePicking(m_ActiveScene, m_MousePickingRenderPass, m_MousePickingPipeline, m_MousePicking2DPipeline, glm::vec2(m_MouseX, (int)(m_ViewportSize.y - m_MouseY - 1)));
            }
        }

        if (m_ShouldOpenAnotherScene)
        {
            OpenScene(m_ScenePathToBeOpened);
            m_ShouldOpenAnotherScene = false;
            m_ScenePathToBeOpened = "";
            m_SceneHierarchyPanel->SetSelectionContext(fbentt::null);
        }
    }

    void EditorLayer::OnDestroy()
    {
        PhysicsEngine::Shutdown();
        Renderer2D::Shutdown();
    }

    void EditorLayer::OnUIRender()
    {
#ifndef FBY_PLATFORM_MACOS
        UI_Menubar();
#endif
        auto& io = ImGui::GetIO();
        if (m_IsCameraMoving)
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        else
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(8.0f * s_OverlayButtonSize.x + 4.0f * s_OverlayPadding, 2 * s_OverlayButtonSize.y));
        m_DidViewportBegin = ImGui::Begin("Viewport");
        ImGui::PopStyleVar(2);

        ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportOffset = ImGui::GetWindowPos();
        m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
        m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        if (m_ViewportSize.x != viewportPanelSize.x || m_ViewportSize.y != viewportPanelSize.y)
            m_HasViewportSizeChanged = true;

        m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
        m_RenderViewportSize = { viewportPanelSize.x * ImGui::GetWindowDpiScale(), viewportPanelSize.y * ImGui::GetWindowDpiScale() };

        m_IsViewportHovered = ImGui::IsWindowHovered();
        // For the Camera Input if other windows are focused but the user right clicks this window then set focus for the camera to continue moving without affecting other windows
        if (m_IsViewportHovered && ImGui::IsMouseDown(ImGuiMouseButton_Right))
            ImGui::SetWindowFocus();
        m_IsViewportFocused = ImGui::IsWindowFocused();

        Application::Get().ImGuiLayerBlockEvents(!m_IsViewportFocused && !m_SceneHierarchyPanel->IsFocused());

        uint32_t imageIndex = VulkanContext::GetCurrentWindow()->GetImageIndex();

        ImGui::Image(reinterpret_cast<ImTextureID>(
            m_ViewportDescriptorSets[imageIndex]),
            ImVec2{ m_ViewportSize.x, m_ViewportSize.y }
        );

        // Scene File Drop Target
        if (ImGui::BeginDragDropTarget() && m_EditorState == EditorState::Edit)
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBY_CONTENT_BROWSER_ITEM"))
            {
                const char* path = (const char*)payload->Data;
                std::filesystem::path filePath{ path };
                const std::string& ext = filePath.extension().string();

                FBY_LOG("Payload recieved: {}, with extension {}", path, ext);

                if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath))
                {
                    if (ext == ".scene" || ext == ".berry")
                    {
                        m_ShouldOpenAnotherScene = true;
                        m_ScenePathToBeOpened = filePath;
                    }
                    else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf") // TODO: Don't limit this to these 3 extensions
                    {
                        const auto& staticMesh = AssetManager::TryGetOrLoadAsset<StaticMesh>(path);
                        auto entity = m_ActiveScene->GetRegistry()->create();
                        m_ActiveScene->GetRegistry()->emplace<IDComponent>(entity);
                        m_ActiveScene->GetRegistry()->emplace<TagComponent>(entity, "StaticMesh");

                        auto& transform = m_ActiveScene->GetRegistry()->emplace<TransformComponent>(entity);
                        constexpr float distance = 5.0f;
                        transform.Translation = m_ActiveCameraController.GetPerspectiveCamera()->GetSpecification().Position + m_ActiveCameraController.GetPerspectiveCamera()->GetSpecification().Direction * distance;

                        m_ActiveScene->GetRegistry()->emplace<MeshComponent>(entity, staticMesh->Handle);
                        m_SceneHierarchyPanel->SetSelectionContext(entity);
                    }
                }
                else
                    FBY_WARN("Bad File given as Scene!");
            }
            ImGui::EndDragDropTarget();
        }

        // ImGuizmo
        const auto& selectedEntity = m_SceneHierarchyPanel->GetSelectionContext();
        if (selectedEntity != fbentt::null && m_GizmoType != -1 && m_EditorState == EditorState::Edit)
        {
            glm::mat4 projectionMatrix = m_ActiveCameraController.GetPerspectiveCamera()->GetProjectionMatrix();
            projectionMatrix[1][1] *= -1;
            glm::mat4 viewMatrix = m_ActiveCameraController.GetPerspectiveCamera()->GetViewMatrix();

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            float windowWidth = (float)ImGui::GetWindowWidth();
            float windowHeight = (float)ImGui::GetWindowHeight();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

            auto& transformComp = m_ActiveScene->GetRegistry()->get<TransformComponent>(selectedEntity);
            glm::mat4 transform = transformComp.CalculateTransform();

            bool snap = Input::IsKeyPressed(KeyCode::LeftControl);
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

                transformComp.Translation = translation;

                glm::vec3 deltaRotation = rotation - transformComp.Rotation;
                transformComp.Rotation += deltaRotation;
                transformComp.Scale = scale;
            }
        }
        ImVec2 workPos = ImGui::GetWindowContentRegionMin();
        workPos.x += ImGui::GetWindowPos().x;
        workPos.y += ImGui::GetWindowPos().y;
        ImVec2 workSize = ImGui::GetWindowSize();
        ImGui::End();

        m_IsAnyOverlayHovered = false;

        m_SceneHierarchyPanel->OnUIRender();

        UI_GizmoOverlay(workPos);
        UI_ToolbarOverlay(workPos, workSize);
        UI_ViewportSettingsOverlay(workPos, workSize);
        UI_BottomPanel();
        // UI_CompositeView();
    }

    void EditorLayer::InvalidateViewportImGuiDescriptorSet(uint32_t index)
    {
        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = Texture2D::GetDefaultSampler();
        desc_image[0].imageView = m_SceneRenderer->GetGeometryPassOutputImageView(index);
        desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write_desc[1] = {};
        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = m_ViewportDescriptorSets[index];
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc[0].pImageInfo = desc_image;
        vkUpdateDescriptorSets(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), 1, write_desc, 0, nullptr);
    }

    void EditorLayer::InvalidateCompositePassImGuiDescriptorSet(uint32_t index)
    {
        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = Texture2D::GetDefaultSampler();
        desc_image[0].imageView = m_SceneRenderer->GetCompositePassOutputImageView(index);
        desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write_desc[1] = {};
        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = m_CompositePassViewportDescriptorSets[index];
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc[0].pImageInfo = desc_image;
        vkUpdateDescriptorSets(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), 1, write_desc, 0, nullptr);
    }

    void EditorLayer::OnEvent(Event& e)
    {
        switch (e.GetType())
        {
            case EventType::MouseButtonPressed:
                this->OnMouseButtonPressedEvent(*(MouseButtonPressedEvent*)(&e));
                break;
            case EventType::KeyPressed:
                this->OnKeyPressedEvent(*(KeyPressedEvent*)(&e));
                break;
            case EventType::MouseScrolled:
                this->OnMouseScrolledEvent(*(MouseScrollEvent*)(&e));
                break;
            case EventType::None:
                break;
        }

        if (m_DidViewportBegin && m_IsViewportHovered)
            m_ActiveCameraController.OnEvent(e);
    }

    void EditorLayer::OnKeyPressedEvent(KeyPressedEvent& e)
    {
        bool ctrl_or_cmd = Input::IsKeyPressed(KeyCode::LeftSuper) || Input::IsKeyPressed(KeyCode::RightSuper);
        bool shift = Input::IsKeyPressed(KeyCode::LeftShift) || Input::IsKeyPressed(KeyCode::RightShift);
        switch (e.Key)
        {
            case KeyCode::D:
                // Duplicate Entity
                if (ctrl_or_cmd && m_EditorState == EditorState::Edit)
                {
                    const auto selectionContext = m_SceneHierarchyPanel->GetSelectionContext();
                    if (selectionContext != fbentt::null)
                    {
                        const auto duplicateEntity = m_ActiveScene->DuplicateEntity(selectionContext);
                        m_SceneHierarchyPanel->SetSelectionContext(duplicateEntity);
                    }
                    break;
                }
            case KeyCode::O:
                if (ctrl_or_cmd)
                {
#ifndef FBY_PLATFORM_MACOS
                    // m_ShouldOpenAnotherScene = true;
                    // m_ScenePathToBeOpened = platform::OpenFile("Flameberry Scene File (*.berry)\0.berry\0");
                    OpenScene();
#endif
                }
                break;
            case KeyCode::S:
                if (ctrl_or_cmd)
                {
#ifndef FBY_PLATFORM_MACOS
                    if (shift)
                        SaveSceneAs();
                    else
                        SaveScene();
#endif
                }
                break;
            case KeyCode::Q:
                if (!m_IsGizmoActive)
                    m_GizmoType = -1;
                break;
            case KeyCode::W:
                if (!m_IsGizmoActive)
                    m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
                break;
            case KeyCode::E:
                if (!m_IsGizmoActive)
                    m_GizmoType = ImGuizmo::OPERATION::ROTATE;
                break;
            case KeyCode::R:
                if (!m_IsGizmoActive)
                    m_GizmoType = ImGuizmo::OPERATION::SCALE;
                break;
            case KeyCode::G:
                if (ctrl_or_cmd)
                    m_EnableGrid = !m_EnableGrid;
                break;
            case KeyCode::Backspace:
                if (ctrl_or_cmd)
                {
                    const auto entity = m_SceneHierarchyPanel->GetSelectionContext();
                    if (entity != fbentt::null)
                    {
                        m_ActiveScene->DestroyEntityTree(entity);
                        m_SceneHierarchyPanel->SetSelectionContext(fbentt::null);
                    }
                }
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

    void EditorLayer::OnMouseScrolledEvent(MouseScrollEvent& e)
    {
    }

    void EditorLayer::OpenProject()
    {
        std::string path = Platform::OpenFile("Flameberry Project File (*.fbproj)\0.fbproj\0");
        if (!path.empty())
            OpenProject(path);
    }

    void EditorLayer::OpenProject(const std::string& path)
    {
        // 1. Unload Assets
        // 2. Unload App Assembly
        // 3. Load Project
        // 4. Load App Assembly
        FBY_ASSERT(0, "Not Implemented Yet!");
    }

    void EditorLayer::SaveScene()
    {
        if (!m_EditorScenePath.empty())
            SceneSerializer::SerializeSceneToFile(m_EditorScenePath.c_str(), m_ActiveScene);
        else
            SaveSceneAs();
    }

    void EditorLayer::SaveSceneAs()
    {
        std::string savePath = Platform::SaveFile("Flameberry Scene File (*.berry)\0.berry\0");
        if (savePath != "")
        {
            SceneSerializer::SerializeSceneToFile(savePath.c_str(), m_ActiveScene);
            m_EditorScenePath = savePath;
            FBY_LOG("Scene saved to path: {}", savePath);
            return;
        }
        FBY_ERROR("Failed to save scene!");
    }

    void EditorLayer::OpenScene()
    {
        std::string sceneToBeLoaded = Platform::OpenFile("Flameberry Scene File (*.berry)\0.berry\0");
        OpenScene(sceneToBeLoaded);
    }

    void EditorLayer::OpenScene(const std::string& path)
    {
        if (path.empty())
        {
            FBY_ERROR("Failed to load scene: path provided is null!");
            return;
        }

        if (SceneSerializer::DeserializeIntoExistingScene(path.c_str(), m_ActiveScene))
        {
            m_EditorScenePath = path;
            m_ActiveScene->OnViewportResize(m_ViewportSize);
            FBY_INFO("Loaded Scene: {}", m_EditorScenePath);
        }
    }

    void EditorLayer::NewScene()
    {
        m_ActiveScene = CreateRef<Scene>();
        m_SceneHierarchyPanel->SetContext(m_ActiveScene);
        m_SceneHierarchyPanel->SetSelectionContext(fbentt::null);
        m_EditorScenePath = "";

        if (m_EditorState == EditorState::Play)
            m_ActiveSceneBackUpCopy = nullptr;
    }

    void EditorLayer::UI_Menubar()
    {
        // Main Menu
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                    SaveSceneAs();
                if (ImGui::MenuItem("Save As", "Ctrl+Shift+S"))
                    SaveSceneAs();
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                    SaveScene();
                if (ImGui::MenuItem("Open", "Ctrl+O"))
                    OpenScene();
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void EditorLayer::UI_Toolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        ImGui::PushStyleColor(ImGuiCol_Button, 0x0);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, Theme::WindowBg);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoScrollWithMouse
            | ImGuiWindowFlags_NoScrollbar;

        ImGuiWindowClass windowClass;
        windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
        ImGui::SetNextWindowClass(&windowClass);

        ImGui::Begin("##Toolbar", nullptr, windowFlags);
        float buttonSize = ImGui::GetContentRegionAvail().y - 8.0f;

        // Center the buttons
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x * 0.5f - 2.0f * buttonSize);
        ImGui::SetCursorPosY(4.0f);

        switch (m_EditorState)
        {
            case EditorState::Edit:
                ImGui::ImageButton("ScenePlayButton", reinterpret_cast<ImTextureID>(m_PlayAndStopIcon->CreateOrGetDescriptorSet()), ImVec2(buttonSize, buttonSize), ImVec2(0, 0), ImVec2(0.5f, 1.0f));
                break;
            case EditorState::Play:
                ImGui::ImageButton("ScenePlayButton", reinterpret_cast<ImTextureID>(m_PlayAndStopIcon->CreateOrGetDescriptorSet()), ImVec2(buttonSize, buttonSize), ImVec2(0, 0), ImVec2(0.5f, 1.0f), ImVec4(0, 0, 0, 0), Theme::AccentColor);
                break;
        }

        if (ImGui::IsItemClicked() && m_EditorState == EditorState::Edit)
            OnScenePlay();

        ImGui::SameLine();

        switch (m_EditorState)
        {
            case EditorState::Edit:
                ImGui::ImageButton("SceneStopButton", reinterpret_cast<ImTextureID>(m_PlayAndStopIcon->CreateOrGetDescriptorSet()), ImVec2(buttonSize, buttonSize), ImVec2(0.5f, 0.0f), ImVec2(1.0f, 1.0f));
                break;
            case EditorState::Play:
                ImGui::ImageButton("SceneStopButton", reinterpret_cast<ImTextureID>(m_PlayAndStopIcon->CreateOrGetDescriptorSet()), ImVec2(buttonSize, buttonSize), ImVec2(0.5f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(0, 0, 0, 0), ImVec4(1, 0, 0, 1));
                break;
        }

        if (ImGui::IsItemClicked() && m_EditorState == EditorState::Play)
            OnSceneEdit();

        ImGui::End();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }

    void EditorLayer::UI_GizmoOverlay(const ImVec2& workPos)
    {
        ImVec2 window_pos;
        window_pos.x = workPos.x + s_OverlayPadding;
        window_pos.y = workPos.y + s_OverlayPadding;

        UI_Overlay("##GizmoOverlay", window_pos, [=]()
            {
                if (ImGui::ImageButton("SelectModeButton", reinterpret_cast<ImTextureID>(m_CursorIcon->CreateOrGetDescriptorSet()), s_OverlayButtonSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), m_GizmoType == -1 ? Theme::AccentColor : ImVec4(1, 1, 1, 1)))
                {
                    m_GizmoType = -1;
                    ImGui::SetWindowFocus("Viewport");
                }
                ImGui::SameLine();
                if (ImGui::ImageButton("TranslateModeButton", reinterpret_cast<ImTextureID>(m_TranslateIcon->CreateOrGetDescriptorSet()), s_OverlayButtonSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), m_GizmoType == ImGuizmo::OPERATION::TRANSLATE ? Theme::AccentColor : ImVec4(1, 1, 1, 1)))
                {
                    m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
                    ImGui::SetWindowFocus("Viewport");
                }
                ImGui::SameLine();
                if (ImGui::ImageButton("RotateModeButton", reinterpret_cast<ImTextureID>(m_RotateIcon->CreateOrGetDescriptorSet()), s_OverlayButtonSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), m_GizmoType == ImGuizmo::OPERATION::ROTATE ? Theme::AccentColor : ImVec4(1, 1, 1, 1)))
                {
                    m_GizmoType = ImGuizmo::OPERATION::ROTATE;
                    ImGui::SetWindowFocus("Viewport");
                }
                ImGui::SameLine();
                if (ImGui::ImageButton("ScaleModeButton", reinterpret_cast<ImTextureID>(m_ScaleIcon->CreateOrGetDescriptorSet()), s_OverlayButtonSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), m_GizmoType == ImGuizmo::OPERATION::SCALE ? Theme::AccentColor : ImVec4(1, 1, 1, 1)))
                {
                    m_GizmoType = ImGuizmo::OPERATION::SCALE;
                    ImGui::SetWindowFocus("Viewport");
                }
            });
    }

    void EditorLayer::UI_ToolbarOverlay(const ImVec2& workPos, const ImVec2& workSize)
    {
        ImVec2 window_pos;
        window_pos.x = workPos.x + workSize.x / 2.0f - 2 * s_OverlayButtonSize.x;
        window_pos.y = workPos.y + s_OverlayPadding;

        UI_Overlay("##ToolbarOverlay", window_pos, [=]()
            {
                switch (m_EditorState)
                {
                    case EditorState::Edit:
                    {
                        if (ImGui::ImageButton("ScenePlayButton", reinterpret_cast<ImTextureID>(m_PlayAndStopIcon->CreateOrGetDescriptorSet()), s_OverlayButtonSize, ImVec2(0, 0), ImVec2(0.5f, 1.0f)))
                            OnScenePlay();
                        break;
                    }
                    case EditorState::Play:
                    {
                        if (m_ActiveScene->IsRuntimePaused())
                        {
                            // UnPause the scene
                            if (ImGui::ImageButton("ScenePlayButton", reinterpret_cast<ImTextureID>(m_PlayAndStopIcon->CreateOrGetDescriptorSet()), s_OverlayButtonSize, ImVec2(0.0f, 0.0f), ImVec2(0.5f, 1.0f), ImVec4(0, 0, 0, 0), Theme::AccentColor))
                                m_ActiveScene->SetRuntimePaused(false);
                        }
                        else
                        {
                            // Pause the scene
                            if (ImGui::ImageButton("ScenePauseButton", reinterpret_cast<ImTextureID>(m_PauseIcon->CreateOrGetDescriptorSet()), s_OverlayButtonSize, ImVec2(0, 0), ImVec2(1.0f, 1.0f), ImVec4(0, 0, 0, 0), Theme::AccentColor))
                                m_ActiveScene->SetRuntimePaused(true);
                        }
                        break;
                    }
                }

                ImGui::SameLine();

                ImGui::BeginDisabled(!m_ActiveScene->IsRuntimePaused());
                if (ImGui::ImageButton("SceneStepButton", reinterpret_cast<ImTextureID>(m_StepIcon->CreateOrGetDescriptorSet()), s_OverlayButtonSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(0, 0, 0, 0)))
                {
                    FBY_LOG("Stepped");
                    m_ActiveScene->Step(1);
                }
                ImGui::EndDisabled();

                ImGui::SameLine();

                switch (m_EditorState)
                {
                    case EditorState::Edit:
                    {
                        ImGui::BeginDisabled();
                        ImGui::ImageButton("SceneStopButton", reinterpret_cast<ImTextureID>(m_PlayAndStopIcon->CreateOrGetDescriptorSet()), s_OverlayButtonSize, ImVec2(0.5f, 0.0f), ImVec2(1.0f, 1.0f));
                        ImGui::EndDisabled();
                        break;
                    }
                    case EditorState::Play:
                    {
                        if (ImGui::ImageButton("SceneStopButton", reinterpret_cast<ImTextureID>(m_PlayAndStopIcon->CreateOrGetDescriptorSet()), s_OverlayButtonSize, ImVec2(0.5f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(0, 0, 0, 0), ImVec4(1, 0, 0, 1)))
                            OnSceneEdit();
                        break;
                    }
                }
            });
    }

    void EditorLayer::UI_ViewportSettingsOverlay(const ImVec2& workPos, const ImVec2& workSize)
    {
        ImVec2 window_pos;
        window_pos.x = workPos.x + workSize.x - 1.5f * s_OverlayPadding - 2 * s_OverlayButtonSize.x;
        window_pos.y = workPos.y + s_OverlayPadding;

        UI_Overlay("##ViewportSettingsOverlay", window_pos, [=]()
            {
                ImGui::ImageButton("##ViewportSettingsButton", reinterpret_cast<ImTextureID>(m_SettingsIcon->CreateOrGetDescriptorSet()), s_OverlayButtonSize, ImVec2(0, 0), ImVec2(1.0f, 1.0f));

                if (ImGui::IsItemClicked())
                    ImGui::OpenPopup("##ViewportSettingsPopup");

                if (ImGui::BeginPopup("##ViewportSettingsPopup"))
                {
                    ImGui::Checkbox("Display Grid", &m_EnableGrid);
                    ImGui::EndPopup();
                }
            });
    }

    void EditorLayer::UI_BottomPanel()
    {
        static bool toggleContentBrowser = false, toggleRendererSettings = false;
        ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoScrollWithMouse
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoScrollbar;

        float height = 2.5f * ImGui::GetFontSize();
        float paddingY = (height - ImGui::GetFontSize()) / 2.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, paddingY));
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, Theme::WindowBgGrey);
        bool begin = ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, height, windowFlags);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        if (begin)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, paddingY));
            begin = ImGui::BeginMenuBar();
            ImGui::PopStyleVar();

            if (begin)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, -1.0f));

                ImGui::PushStyleColor(ImGuiCol_Button, Theme::DarkThemeColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::DarkThemeColor);

                ImGui::SetCursorPosX(0.0f);

                if (ImGui::Button(ICON_LC_FOLDER_TREE"  Content Browser", ImVec2(0.0f, -1.0f)))
                    toggleContentBrowser = !toggleContentBrowser;

                ImGui::SameLine();

                if (ImGui::Button(ICON_LC_SETTINGS"  Renderer Settings", ImVec2(0.0f, -1.0f)))
                    toggleRendererSettings = !toggleRendererSettings;

                ImGui::PopStyleColor(2);
                ImGui::EndMenuBar();
            }
            ImGui::PopStyleVar(4);
        }
        ImGui::End();

        if (toggleContentBrowser)
            m_ContentBrowserPanel->OnUIRender();
        if (toggleRendererSettings)
            UI_RendererSettings();
    }
    void EditorLayer::UI_CompositeView()
    {
        // Display composited framebuffer
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Composite Result");
        ImGui::PopStyleVar();

        ImVec2 compositeViewportSize = ImGui::GetContentRegionAvail();

        ImGui::Image(reinterpret_cast<ImTextureID>(
            m_CompositePassViewportDescriptorSets[VulkanContext::GetCurrentWindow()->GetSwapChain()->GetAcquiredImageIndex()]),
            ImVec2{ compositeViewportSize.x, compositeViewportSize.y }
        );
        ImGui::End();
    }

    void EditorLayer::UI_RendererSettings()
    {
        constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoKeepColumnsVisible;

        ImGui::Begin("Renderer Settings");

        if (ImGui::CollapsingHeader("Renderer Frame Statistics (Geometry Pass Only)", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed))
        {
            ImGui::TextWrapped("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            FBY_DISPLAY_SCOPE_DETAILS_IMGUI();

            const auto& rendererFrameStats = Renderer::GetRendererFrameStats();
            // ImGui::Text("Mesh Count: %u", rendererFrameStats.MeshCount);
            // ImGui::Text("SubMesh Count: %u", rendererFrameStats.SubMeshCount);
            ImGui::Text("Bound Materials: %u", rendererFrameStats.BoundMaterials);
            ImGui::Text("Vertex and IndexBuffer State Switches: %u", rendererFrameStats.VertexAndIndexBufferStateSwitches);
            // ImGui::Text("Mesh Draw Calls: %u", rendererFrameStats.DrawCallCount);
            // ImGui::Text("Indices: %u", rendererFrameStats.IndexCount);
        }

        if (ImGui::CollapsingHeader("Scene Renderer", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed))
        {
            if (ImGui::BeginTable("RendererSettings_Attributes", 2, flags))
            {
                ImGui::TableSetupColumn("Attribute_Name", ImGuiTableColumnFlags_WidthFixed, 100);
                ImGui::TableSetupColumn("Attribute_Value", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                auto& settings = m_SceneRenderer->GetRendererSettingsRef();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Mesh Shader");
                ImGui::TableNextColumn();

                ImGui::Button("Reload");
                if (ImGui::IsItemClicked())
                    m_ShouldReloadMeshShaders = true;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Frustum Culling");
                ImGui::TableNextColumn();
                ImGui::Checkbox("##Frustum_Culling", &settings.FrustumCulling);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Show Bounding Boxes");
                ImGui::TableNextColumn();
                ImGui::Checkbox("##Show_Bounding_Boxes", &settings.ShowBoundingBoxes);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Enable Shadows");
                ImGui::TableNextColumn();
                ImGui::Checkbox("##Enable_Shadows", &settings.EnableShadows);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Show Cascades");
                ImGui::TableNextColumn();
                ImGui::Checkbox("##Show_Cascades", &settings.ShowCascades);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Soft Shadows");
                ImGui::TableNextColumn();
                ImGui::Checkbox("##Soft_Shadows", &settings.SoftShadows);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Lambda Split");
                ImGui::TableNextColumn();
                FBY_PUSH_WIDTH_MAX(ImGui::DragFloat("##Lambda_Split", &settings.CascadeLambdaSplit, 0.001f, 0.0f, 1.0f));
                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void EditorLayer::OnSceneEdit()
    {
        m_EditorState = EditorState::Edit;

        m_ActiveScene->OnStopRuntime();

        // Delete the m_RuntimeScene
        std::swap(m_ActiveScene, m_ActiveSceneBackUpCopy);
        m_SceneHierarchyPanel->SetContext(m_ActiveScene);
        m_ActiveSceneBackUpCopy = nullptr;
    }

    void EditorLayer::OnScenePlay()
    {
        m_EditorState = EditorState::Play;

        std::swap(m_ActiveScene, m_ActiveSceneBackUpCopy);
        // Copy m_ActiveSceneBackUpCopy to m_ActiveScene
        m_ActiveScene = CreateRef<Scene>(m_ActiveSceneBackUpCopy);
        m_SceneHierarchyPanel->SetContext(m_ActiveScene);

        m_ActiveScene->OnStartRuntime();
    }

}
