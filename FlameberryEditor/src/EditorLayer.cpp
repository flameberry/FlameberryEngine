#include "EditorLayer.h"

#include "ImGuizmo/ImGuizmo.h"
#include "Renderer/Vulkan/Framebuffer.h"
#include "Utils.h"

namespace Flameberry {
    struct CameraUniformBufferObject { glm::mat4 ViewMatrix, ProjectionMatrix, ViewProjectionMatrix; };
    EditorLayer::EditorLayer(const std::string_view& projectPath)
        : m_ProjectPath(projectPath), m_ActiveCameraController(PerspectiveCameraSpecification{
            glm::vec3(0.0f, 2.0f, 4.0f),
            glm::vec3(0.0f, -0.3f, -1.0f),
            (float)Application::Get().GetWindow().GetWidth() / (float)Application::Get().GetWindow().GetHeight(),
            45.0f,
            0.1f,
            1000.0f
            }
        )
    {
#ifdef __APPLE__
        platform::SetSaveSceneCallbackMenuBar(FL_BIND_EVENT_FN(EditorLayer::SaveScene));
        platform::SetSaveSceneAsCallbackMenuBar(FL_BIND_EVENT_FN(EditorLayer::SaveSceneAs));
        platform::SetOpenSceneCallbackMenuBar(FL_BIND_EVENT_FN(EditorLayer::OpenScene));
        platform::CreateMenuBar();
#endif
    }

    void EditorLayer::OnCreate()
    {
        m_VkTextureSampler = Texture2D::GetDefaultSampler();
        auto swapchain = VulkanContext::GetCurrentWindow()->GetSwapChain();
        uint32_t imageCount = swapchain->GetSwapChainImageCount();

        {
            FramebufferSpecification shadowMapFramebufferSpec;
            shadowMapFramebufferSpec.Width = SHADOW_MAP_SIZE;
            shadowMapFramebufferSpec.Height = SHADOW_MAP_SIZE;
            shadowMapFramebufferSpec.Attachments = { { VK_FORMAT_D32_SFLOAT, SHADOW_MAP_CASCADE_COUNT } };
            shadowMapFramebufferSpec.Samples = 1;
            shadowMapFramebufferSpec.DepthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            shadowMapFramebufferSpec.DepthStencilClearValue = { 1.0f, 0 };

            RenderPassSpecification shadowMapRenderPassSpec;

            shadowMapRenderPassSpec.TargetFramebuffers.resize(imageCount);

            for (uint32_t i = 0; i < imageCount; i++)
                shadowMapRenderPassSpec.TargetFramebuffers[i] = Framebuffer::Create(shadowMapFramebufferSpec);

            m_ShadowMapRenderPass = RenderPass::Create(shadowMapRenderPassSpec);
        }
        CreateShadowMapPipeline();

        m_CursorIcon = Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/cursor_icon.png");
        m_CursorIconActive = Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/cursor_icon_active.png");
        m_TranslateIcon = Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/translate_icon_2.png");
        m_TranslateIconActive = Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/translate_icon_2_active.png");
        m_RotateIcon = Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/rotate_icon.png");
        m_RotateIconActive = Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/rotate_icon_active.png");
        m_ScaleIcon = Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/scale_icon.png");
        m_ScaleIconActive = Texture2D::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/scale_icon_active.png");

        // Creating Uniform Buffers
        VkDeviceSize uniformBufferSize = sizeof(CameraUniformBufferObject);

        BufferSpecification uniformBufferSpec;
        uniformBufferSpec.InstanceCount = 1;
        uniformBufferSpec.InstanceSize = uniformBufferSize;
        uniformBufferSpec.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        uniformBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        for (auto& uniformBuffer : m_UniformBuffers)
        {
            uniformBuffer = std::make_unique<Buffer>(uniformBufferSpec);
            uniformBuffer->MapMemory(uniformBufferSize);
        }

        // Creating Descriptors
        DescriptorSetLayoutSpecification cameraBufferDescLayoutSpec;
        cameraBufferDescLayoutSpec.Bindings.emplace_back();

        cameraBufferDescLayoutSpec.Bindings[0].binding = 0;
        cameraBufferDescLayoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cameraBufferDescLayoutSpec.Bindings[0].descriptorCount = 1;
        cameraBufferDescLayoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        m_CameraBufferDescSetLayout = DescriptorSetLayout::Create(cameraBufferDescLayoutSpec);

        DescriptorSetSpecification cameraBufferDescSetSpec;
        cameraBufferDescSetSpec.Layout = m_CameraBufferDescSetLayout;

        m_CameraBufferDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_CameraBufferDescriptorSets[i] = DescriptorSet::Create(cameraBufferDescSetSpec);

            VkDescriptorBufferInfo vk_descriptor_buffer_info{};
            vk_descriptor_buffer_info.buffer = m_UniformBuffers[i]->GetBuffer();
            vk_descriptor_buffer_info.offset = 0;
            vk_descriptor_buffer_info.range = sizeof(CameraUniformBufferObject);

            m_CameraBufferDescriptorSets[i]->WriteBuffer(0, vk_descriptor_buffer_info);
            m_CameraBufferDescriptorSets[i]->Update();
        }

        m_Registry = std::make_shared<fbentt::registry>();
        m_ActiveScene = Scene::Create(m_Registry);

        m_SceneHierarchyPanel = SceneHierarchyPanel::Create(m_ActiveScene);
        m_ContentBrowserPanel = ContentBrowserPanel::Create(m_ProjectPath);
        m_EnvironmentSettingsPanel = EnvironmentSettingsPanel::Create(m_ActiveScene);

        BufferSpecification mousePickingBufferSpec;
        mousePickingBufferSpec.InstanceCount = 1;
        mousePickingBufferSpec.InstanceSize = sizeof(int32_t);
        mousePickingBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        mousePickingBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        m_MousePickingBuffer = std::make_unique<Buffer>(mousePickingBufferSpec);

        {
            VkFormat swapChainImageFormat = swapchain->GetSwapChainImageFormat();
            VkSampleCountFlagBits sampleCount = RenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

            FramebufferSpecification sceneFramebufferSpec;
            sceneFramebufferSpec.Width = m_ViewportSize.x;
            sceneFramebufferSpec.Height = m_ViewportSize.y;
            sceneFramebufferSpec.Attachments = { swapChainImageFormat, VulkanSwapChain::GetDepthFormat() };
            sceneFramebufferSpec.Samples = sampleCount;
            sceneFramebufferSpec.ClearColorValue = { 0.0f, 0.0f, 0.0f, 1.0f };
            sceneFramebufferSpec.DepthStencilClearValue = { 1.0f, 0 };
            // Used to not store multisample color attachment
            sceneFramebufferSpec.ColorStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            // For outline compositing
            sceneFramebufferSpec.StencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

            RenderPassSpecification sceneRenderPassSpec;

            sceneRenderPassSpec.TargetFramebuffers.resize(imageCount);
            for (uint32_t i = 0; i < imageCount; i++)
                sceneRenderPassSpec.TargetFramebuffers[i] = Framebuffer::Create(sceneFramebufferSpec);

            m_SceneRenderPass = RenderPass::Create(sceneRenderPassSpec);
        }

        {
            FramebufferSpecification mousePickingFramebufferSpec;
            mousePickingFramebufferSpec.Width = m_ViewportSize.x;
            mousePickingFramebufferSpec.Height = m_ViewportSize.y;
            mousePickingFramebufferSpec.Samples = 1;
            mousePickingFramebufferSpec.Attachments = { VK_FORMAT_R32_SINT, VK_FORMAT_D32_SFLOAT };
            mousePickingFramebufferSpec.ClearColorValue = { .int32 = { -1 } };
            mousePickingFramebufferSpec.DepthStencilClearValue = { 1.0f, 0 };

            RenderPassSpecification mousePickingRenderPassSpec;
            mousePickingRenderPassSpec.TargetFramebuffers = { Framebuffer::Create(mousePickingFramebufferSpec) };

            m_MousePickingRenderPass = RenderPass::Create(mousePickingRenderPassSpec);
        }

        CreateMousePickingPipeline();
        CreateCompositePipeline();

        std::vector<VkImageView> views;
        for (const auto& framebuffer : m_ShadowMapRenderPass->GetSpecification().TargetFramebuffers)
            views.push_back(framebuffer->GetDepthAttachment()->GetImageView());
        m_SceneRenderer = std::make_unique<SceneRenderer>(m_CameraBufferDescSetLayout->GetLayout(), m_SceneRenderPass, views, m_ShadowMapSampler);

        m_ViewportDescriptorSets.resize(imageCount);
        for (int i = 0; i < imageCount; i++)
        {
            m_ViewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                m_VkTextureSampler,
                m_SceneRenderPass->GetSpecification().TargetFramebuffers[i]->GetColorResolveAttachment(0)->GetImageView(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
        }

        m_CompositePassViewportDescriptorSets.resize(imageCount);
        for (int i = 0; i < imageCount; i++)
        {
            m_CompositePassViewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                m_VkTextureSampler,
                m_CompositePass->GetSpecification().TargetFramebuffers[i]->GetColorAttachment(0)->GetImageView(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
        }

        Renderer2D::Init(m_CameraBufferDescSetLayout->GetLayout(), m_SceneRenderPass);
    }

    void EditorLayer::OnUpdate(float delta)
    {
        FL_PROFILE_SCOPE("Editor_OnUpdate");

        // Update all image index related descriptors or framebuffers here
        Renderer::Submit([&](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                if (m_SceneRenderPass->GetSpecification().TargetFramebuffers[imageIndex]->Resize(m_ViewportSize.x, m_ViewportSize.y, m_SceneRenderPass->GetRenderPass()))
                {
                    InvalidateViewportImGuiDescriptorSet(imageIndex);

                    m_CompositePassDescriptorSets[imageIndex]->WriteImage(0, {
                         .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                         .imageView = m_SceneRenderPass->GetSpecification().TargetFramebuffers[imageIndex]->GetColorResolveAttachment(0)->GetImageView(),
                         .sampler = m_VkTextureSampler
                        }
                    );

                    // m_CompositePassDescriptorSets[imageIndex]->WriteImage(1, {
                    //      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                    //       .imageView = m_StencilImages[imageIndex]->GetImageView(),
                    //       .sampler = m_ShadowMapSampler
                    //     }
                    // );

                    m_CompositePassDescriptorSets[imageIndex]->Update();
                }
                if (m_CompositePass->GetSpecification().TargetFramebuffers[imageIndex]->Resize(m_ViewportSize.x, m_ViewportSize.y, m_CompositePass->GetRenderPass()))
                    InvalidateCompositePassImGuiDescriptorSet(imageIndex);

                VkClearColorValue clearColor = { m_ActiveScene->GetClearColor().x, m_ActiveScene->GetClearColor().y, m_ActiveScene->GetClearColor().z, 1.0f };
                m_SceneRenderPass->GetSpecification().TargetFramebuffers[imageIndex]->SetClearColorValue(clearColor);
            }
        );

        uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();

        bool didCameraResize = m_ActiveCameraController.GetPerspectiveCamera()->OnResize(m_ViewportSize.x / m_ViewportSize.y);
        if (m_IsViewportFocused)
            m_IsCameraMoving = m_ActiveCameraController.OnUpdate(delta);

        UpdateShadowMapCascades(); // TODO: Update only when camera or directional light is updated

        {
            FL_PROFILE_SCOPE("Shadow Pass");
            m_ShadowMapRenderPass->Begin();
            m_ShadowMapPipeline->Bind();

            m_ShadowMapUniformBuffers[currentFrameIndex]->WriteToBuffer(m_CascadeMatrices.data(), sizeof(glm::mat4) * SHADOW_MAP_CASCADE_COUNT, 0);

            auto shadowMapDesc = m_ShadowMapDescriptorSets[currentFrameIndex]->GetDescriptorSet();
            const auto& shadowMapPipelineLayout = m_ShadowMapPipelineLayout;

            Renderer::Submit([shadowMapDesc, shadowMapPipelineLayout](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                {
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipelineLayout, 0, 1, &shadowMapDesc, 0, nullptr);
                }
            );

            m_SceneRenderer->OnDrawForShadowPass(m_ShadowMapPipelineLayout, m_ActiveScene);
            m_ShadowMapRenderPass->End();
        }

        {
            FL_PROFILE_SCOPE("Viewport Render Pass");
            m_SceneRenderPass->Begin();

            RenderCommand::SetViewport(0.0f, 0.0f, m_ViewportSize.x, m_ViewportSize.y);
            RenderCommand::SetScissor({ 0, 0 }, VkExtent2D{ (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y });

            CameraUniformBufferObject uniformBufferObject{};
            uniformBufferObject.ViewMatrix = m_ActiveCameraController.GetPerspectiveCamera()->GetViewMatrix();
            uniformBufferObject.ProjectionMatrix = m_ActiveCameraController.GetPerspectiveCamera()->GetProjectionMatrix();
            uniformBufferObject.ViewProjectionMatrix = m_ActiveCameraController.GetPerspectiveCamera()->GetViewProjectionMatrix();

            m_UniformBuffers[currentFrameIndex]->WriteToBuffer(&uniformBufferObject, sizeof(uniformBufferObject), 0);

            if (m_EnableGrid)
                Renderer2D::AddGrid(25);

            m_SceneRenderer->OnDraw(m_CameraBufferDescriptorSets[currentFrameIndex]->GetDescriptorSet(), *m_ActiveCameraController.GetPerspectiveCamera(), m_ActiveScene, m_ViewportSize, m_CascadeMatrices, m_CascadeDepthSplits, m_ColorCascades, m_SceneHierarchyPanel->GetSelectionContext());

            Renderer2D::Render(m_CameraBufferDescriptorSets[currentFrameIndex]->GetDescriptorSet());
            m_SceneRenderPass->End();
        }

        {
            // Composite pass
            m_CompositePass->Begin();
            m_CompositePipeline->Bind();

            auto pipelineLayout = m_CompositePipelineLayout;
            std::vector<VkDescriptorSet> descriptorSets(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
            for (uint8_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
                descriptorSets[i] = m_CompositePassDescriptorSets[i]->GetDescriptorSet();

            Renderer::Submit([pipelineLayout, descriptorSets](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                {
                    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);
                    vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
                }
            );
            m_CompositePass->End();
        }

        if (m_IsMousePickingBufferReady)
        {
            RenderCommand::WritePixelFromImageToBuffer(
                m_MousePickingBuffer->GetBuffer(),
                m_MousePickingRenderPass->GetSpecification().TargetFramebuffers[0]->GetColorAttachment(0)->GetImage(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                { m_MouseX, m_ViewportSize.y - m_MouseY }
            );

            m_MousePickingBuffer->MapMemory(sizeof(int32_t));
            int32_t* data = (int32_t*)m_MousePickingBuffer->GetMappedMemory();
            int32_t entityIndex = data[0];
            m_MousePickingBuffer->UnmapMemory();
            m_SceneHierarchyPanel->SetSelectionContext((entityIndex != -1) ? m_Registry->get_entity_at_index(entityIndex) : fbentt::null);
            // FL_LOG("Selected Entity Index: {0}", entityIndex);
            m_IsMousePickingBufferReady = false;
        }

        // bool attemptedToMoveCamera = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
        bool attemptedToSelect = ImGui::IsMouseClicked(ImGuiMouseButton_Left)
            && m_DidViewportBegin
            && m_IsViewportFocused
            && !m_IsGizmoOverlayHovered
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
                FL_PROFILE_SCOPE("Last Mouse Picking Pass");
                m_IsMousePickingBufferReady = true;

                bool shouldResize = m_MousePickingRenderPass->GetSpecification().TargetFramebuffers[0]->Resize(m_ViewportSize.x, m_ViewportSize.y, m_MousePickingRenderPass->GetRenderPass());

                m_MousePickingRenderPass->Begin(0, { m_MouseX, (int)(m_ViewportSize.y - m_MouseY) }, { 1, 1 });
                m_MousePickingPipeline->Bind();
                auto descSet = m_CameraBufferDescriptorSets[currentFrameIndex]->GetDescriptorSet();
                const auto& mousePickingPipelineLayout = m_MousePickingPipelineLayout;
                Renderer::Submit([descSet, mousePickingPipelineLayout](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
                    {
                        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mousePickingPipelineLayout, 0, 1, &descSet, 0, nullptr);
                    }
                );
                m_SceneRenderer->OnDrawForMousePickingPass(m_MousePickingPipelineLayout, m_ActiveScene);
                m_MousePickingRenderPass->End();
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
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        Renderer2D::Destroy();
        vkDestroySampler(device, m_ShadowMapSampler, nullptr);
        vkDestroyPipelineLayout(device, m_ShadowMapPipelineLayout, nullptr);
        vkDestroyPipelineLayout(device, m_MousePickingPipelineLayout, nullptr);
        vkDestroyPipelineLayout(device, m_CompositePipelineLayout, nullptr);
    }

    void EditorLayer::OnUIRender()
    {
#ifndef __APPLE__
        ShowMenuBar();
#endif
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        m_DidViewportBegin = ImGui::Begin("Viewport");

        ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportOffset = ImGui::GetWindowPos();
        m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
        m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

        m_IsViewportFocused = ImGui::IsWindowFocused();
        m_IsViewportHovered = ImGui::IsWindowHovered();

        uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();
        uint32_t imageIndex = VulkanContext::GetCurrentWindow()->GetImageIndex();

        ImGui::Image(reinterpret_cast<ImTextureID>(
            m_ViewportDescriptorSets[imageIndex]),
            ImVec2{ m_ViewportSize.x, m_ViewportSize.y }
        );

        // Scene File Drop Target
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FL_CONTENT_BROWSER_ITEM"))
            {
                const char* path = (const char*)payload->Data;
                std::filesystem::path filePath{ path };
                const std::string& ext = filePath.extension().string();

                FL_LOG("Payload recieved: {0}, with extension {1}", path, ext);

                if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath)) {
                    if (ext == ".scene" || ext == ".berry") {
                        m_ShouldOpenAnotherScene = true;
                        m_ScenePathToBeOpened = filePath;
                    }
                    else if (ext == ".obj") {
                        const auto& staticMesh = AssetManager::TryGetOrLoadAsset<StaticMesh>(path);
                        auto entity = m_Registry->create();
                        m_Registry->emplace<IDComponent>(entity);
                        m_Registry->emplace<TagComponent>(entity, "StaticMesh");
                        m_Registry->emplace<TransformComponent>(entity);
                        m_Registry->emplace<MeshComponent>(entity, staticMesh->Handle);
                        m_SceneHierarchyPanel->SetSelectionContext(entity);
                    }
                }
                else
                    FL_WARN("Bad File given as Scene!");
            }
            ImGui::EndDragDropTarget();
        }

        // ImGuizmo
        const auto& selectedEntity = m_SceneHierarchyPanel->GetSelectionContext();
        if (selectedEntity != fbentt::null && m_GizmoType != -1)
        {
            glm::mat4 projectionMatrix = m_ActiveCameraController.GetPerspectiveCamera()->GetProjectionMatrix();
            projectionMatrix[1][1] *= -1;
            const auto& viewMatrix = m_ActiveCameraController.GetPerspectiveCamera()->GetViewMatrix();

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            float windowWidth = (float)ImGui::GetWindowWidth();
            float windowHeight = (float)ImGui::GetWindowHeight();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

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
        ImVec2 work_pos = ImGui::GetWindowContentRegionMin();
        work_pos.x += ImGui::GetWindowPos().x;
        work_pos.y += ImGui::GetWindowPos().y;
        ImGui::End();

        if (m_DidViewportBegin)
        {
            ImVec2 overlayButtonSize = { 20, 20 };

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
                | ImGuiWindowFlags_AlwaysAutoResize
                | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoFocusOnAppearing
                | ImGuiWindowFlags_NoNav;
            const float PAD = 10.0f;
            ImVec2 window_pos;
            window_pos.x = work_pos.x + PAD;
            window_pos.y = work_pos.y + PAD;
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
            window_flags |= ImGuiWindowFlags_NoMove;

            ImGui::SetNextWindowBgAlpha(0.45f); // Transparent background
            ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, overlayButtonSize);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 5.0f, 3.0f });
            ImGui::Begin("##GizmoOverlay", __null, window_flags);

            m_IsGizmoOverlayHovered = ImGui::IsWindowHovered();

            ImGui::PopStyleVar(4);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_GizmoType == -1 ? m_CursorIconActive->GetDescriptorSet() : m_CursorIcon->GetDescriptorSet()), overlayButtonSize))
            {
                m_GizmoType = -1;
                ImGui::SetWindowFocus("Viewport");
            }
            ImGui::SameLine();
            if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_GizmoType == ImGuizmo::OPERATION::TRANSLATE ? m_TranslateIconActive->GetDescriptorSet() : m_TranslateIcon->GetDescriptorSet()), overlayButtonSize))
            {
                m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
                ImGui::SetWindowFocus("Viewport");
            }
            ImGui::SameLine();
            if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_GizmoType == ImGuizmo::OPERATION::ROTATE ? m_RotateIconActive->GetDescriptorSet() : m_RotateIcon->GetDescriptorSet()), overlayButtonSize))
            {
                m_GizmoType = ImGuizmo::OPERATION::ROTATE;
                ImGui::SetWindowFocus("Viewport");
            }
            ImGui::SameLine();
            if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(m_GizmoType == ImGuizmo::OPERATION::SCALE ? m_ScaleIconActive->GetDescriptorSet() : m_ScaleIcon->GetDescriptorSet()), overlayButtonSize))
            {
                m_GizmoType = ImGuizmo::OPERATION::SCALE;
                ImGui::SetWindowFocus("Viewport");
            }
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
            ImGui::End();
        }
        ImGui::PopStyleVar();

        ImGui::Begin("Settings");
        ImGui::TextWrapped("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        FL_DISPLAY_SCOPE_DETAILS_IMGUI();
        ImGui::Separator();
        ImGui::Checkbox("Color Cascades", &m_ColorCascades);
        ImGui::DragFloat("Lambda Split", &m_LambdaSplit, 0.002f, 0.0f, 1.0f);
        ImGui::End();

        // Display composited framebuffer
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Composite Result");
        ImGui::PopStyleVar();

        ImVec2 compositeViewportSize = ImGui::GetContentRegionAvail();

        ImGui::Image(reinterpret_cast<ImTextureID>(
            m_CompositePassViewportDescriptorSets[imageIndex]),
            ImVec2{ compositeViewportSize.x, compositeViewportSize.y }
        );
        ImGui::End();

        m_SceneHierarchyPanel->OnUIRender();
        m_ContentBrowserPanel->OnUIRender();
        m_EnvironmentSettingsPanel->OnUIRender();
    }

    void EditorLayer::InvalidateViewportImGuiDescriptorSet(uint32_t index)
    {
        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = m_VkTextureSampler;
        desc_image[0].imageView = m_SceneRenderPass->GetSpecification().TargetFramebuffers[index]->GetColorResolveAttachment(0)->GetImageView();
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
        desc_image[0].sampler = m_VkTextureSampler;
        desc_image[0].imageView = m_CompositePass->GetSpecification().TargetFramebuffers[index]->GetColorAttachment(0)->GetImageView();
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
        case EventType::MOUSEBUTTON_PRESSED:
            this->OnMouseButtonPressedEvent(*(MouseButtonPressedEvent*)(&e));
            break;
        case EventType::KEY_PRESSED:
            this->OnKeyPressedEvent(*(KeyPressedEvent*)(&e));
            break;
        case EventType::MOUSE_SCROLL:
            this->OnMouseScrolledEvent(*(MouseScrollEvent*)(&e));
            break;
        case EventType::NONE:
            break;
        }

        if (m_DidViewportBegin && m_IsViewportHovered)
            m_ActiveCameraController.OnEvent(e);
    }

    void EditorLayer::OnKeyPressedEvent(KeyPressedEvent& e)
    {
        bool ctrl_or_cmd = Input::IsKey(GLFW_KEY_LEFT_SUPER, GLFW_PRESS);
        bool shift = Input::IsKey(GLFW_KEY_LEFT_SHIFT, GLFW_PRESS) || Input::IsKey(GLFW_KEY_RIGHT_SHIFT, GLFW_PRESS);
        switch (e.KeyCode)
        {
        case GLFW_KEY_O:
            if (ctrl_or_cmd) {
#ifndef __APPLE__
                // m_ShouldOpenAnotherScene = true;
                // m_ScenePathToBeOpened = platform::OpenFile();
                OpenScene();
#endif
            }
            break;
        case GLFW_KEY_S:
            if (ctrl_or_cmd)
            {
#ifndef __APPLE__
                if (shift || m_OpenedScenePathIfExists.empty())
                    SaveScene();
                else
                    SaveScene(m_OpenedScenePathIfExists);
#endif
            }
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
        case GLFW_KEY_G:
            if (ctrl_or_cmd)
                m_EnableGrid = !m_EnableGrid;
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

    void EditorLayer::SaveScene()
    {
        if (!m_OpenedScenePathIfExists.empty())
            SceneSerializer::SerializeSceneToFile(m_OpenedScenePathIfExists.c_str(), m_ActiveScene);
        else
            SaveSceneAs();
    }

    void EditorLayer::SaveSceneAs()
    {
        std::string savePath = platform::SaveFile();
        if (savePath != "")
        {
            SceneSerializer::SerializeSceneToFile(savePath.c_str(), m_ActiveScene);
            FL_LOG("Scene saved to path: {0}", savePath);
            return;
        }
        FL_ERROR("Failed to save scene!");
    }

    void EditorLayer::OpenScene()
    {
        std::string sceneToBeLoaded = platform::OpenFile();
        if (sceneToBeLoaded != "")
        {
            if (SceneSerializer::DeserializeIntoExistingScene(sceneToBeLoaded.c_str(), m_ActiveScene))
            {
                m_OpenedScenePathIfExists = sceneToBeLoaded;
                FL_INFO("Loaded Scene: {0}", sceneToBeLoaded);
                return;
            }
        }
        FL_ERROR("Failed to load scene!");
    }

    void EditorLayer::OpenScene(const std::string& path)
    {
        if (!path.empty())
        {
            if (SceneSerializer::DeserializeIntoExistingScene(path.c_str(), m_ActiveScene))
                m_OpenedScenePathIfExists = path;
        }
    }

    void EditorLayer::NewScene()
    {
        FL_ASSERT(0, "Not implemented yet!");
    }

    void EditorLayer::UpdateShadowMapCascades()
    {
        float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

        const float nearClip = m_ActiveCameraController.GetPerspectiveCamera()->GetSpecification().zNear;
        const float farClip = m_ActiveCameraController.GetPerspectiveCamera()->GetSpecification().zFar;
        const float clipRange = farClip - nearClip;

        const float minZ = nearClip;
        const float maxZ = nearClip + clipRange;

        const float range = maxZ - minZ;
        const float ratio = maxZ / minZ;

        // const float cascadeSplitLambda = 0.91f;
        const float cascadeSplitLambda = m_LambdaSplit;

        // Calculate split depths based on view camera frustum
        // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
        for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
            const float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
            const float log = minZ * std::pow(ratio, p);
            const float uniform = minZ + range * p;
            const float d = cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }

        // Calculate orthographic projection matrix for each cascade
        float lastSplitDist = 0.0;

        // Project frustum corners into world space
        const glm::mat4 invCam = glm::inverse(m_ActiveCameraController.GetPerspectiveCamera()->GetViewProjectionMatrix());

        for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
            const float splitDist = cascadeSplits[i];

            glm::vec3 frustumCorners[8] = {
                glm::vec3(-1.0f,  1.0f, 0.0f),
                glm::vec3(1.0f,   1.0f, 0.0f),
                glm::vec3(1.0f,  -1.0f, 0.0f),
                glm::vec3(-1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f,  1.0f, 1.0f),
                glm::vec3(1.0f,   1.0f, 1.0f),
                glm::vec3(1.0f,  -1.0f, 1.0f),
                glm::vec3(-1.0f, -1.0f, 1.0f),
            };

            for (uint32_t i = 0; i < 8; i++) {
                glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
                frustumCorners[i] = invCorner / invCorner.w;
            }

            for (uint32_t i = 0; i < 4; i++) {
                glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
                frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
                frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
            }

            // Get frustum center
            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for (uint32_t i = 0; i < 8; i++) {
                frustumCenter += frustumCorners[i];
            }
            frustumCenter /= 8.0f;

            float radius = 0.0f;
            for (uint32_t i = 0; i < 8; i++) {
                float distance = glm::length(frustumCorners[i] - frustumCenter);
                radius = glm::max(radius, distance);
            }
            radius = std::ceil(radius * 16.0f) / 16.0f;

            const glm::vec3 maxExtents = glm::vec3(radius);
            const glm::vec3 minExtents = -maxExtents;

            const glm::vec3 lightDir = glm::normalize(m_ActiveScene->GetDirectionalLight().Direction);
            const glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

            // Store split distance and matrix in cascade
            m_CascadeDepthSplits[i] = (m_ActiveCameraController.GetPerspectiveCamera()->GetSpecification().zNear + splitDist * clipRange) * -1.0f;
            m_CascadeMatrices[i] = lightOrthoMatrix * lightViewMatrix;

            lastSplitDist = cascadeSplits[i];
        }
    }

    void EditorLayer::ShowMenuBar()
    {
        // Main Menu
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save As", "Cmd+Shift+S"))
                    SaveSceneAs();
                if (ImGui::MenuItem("Save", "Cmd+S"))
                    SaveScene();
                if (ImGui::MenuItem("Open", "Cmd+O"))
                    OpenScene();
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void EditorLayer::CreateMousePickingPipeline()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        // Creating Descriptors
        DescriptorSetLayoutSpecification mousePickingDescSetLayoutSpec;
        mousePickingDescSetLayoutSpec.Bindings.emplace_back();
        mousePickingDescSetLayoutSpec.Bindings[0].binding = 0;
        mousePickingDescSetLayoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        mousePickingDescSetLayoutSpec.Bindings[0].descriptorCount = 1;
        mousePickingDescSetLayoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        mousePickingDescSetLayoutSpec.Bindings[0].pImmutableSamplers = nullptr;

        m_MousePickingDescriptorSetLayout = DescriptorSetLayout::Create(mousePickingDescSetLayoutSpec);

        // Pipeline Creation
        VkPushConstantRange vk_push_constant_range{};
        vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        vk_push_constant_range.size = sizeof(MousePickingPushConstantData);
        vk_push_constant_range.offset = 0;

        VkDescriptorSetLayout descriptorSetLayouts[] = { m_MousePickingDescriptorSetLayout->GetLayout() };
        VkPushConstantRange pushConstantRanges[] = { vk_push_constant_range };

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = 1;
        vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
        vk_pipeline_layout_create_info.pushConstantRangeCount = 1;
        vk_pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_MousePickingPipelineLayout));

        PipelineSpecification pipelineSpec{};
        pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/mouse_picking.vert.spv";
        pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/mouse_picking.frag.spv";

        pipelineSpec.PipelineLayout = m_MousePickingPipelineLayout;
        pipelineSpec.RenderPass = m_MousePickingRenderPass;

        pipelineSpec.VertexLayout = { VertexInputAttribute::VEC3F };
        pipelineSpec.VertexInputBindingDescription = MeshVertex::GetBindingDescription();

        m_MousePickingPipeline = Pipeline::Create(pipelineSpec);
    }

    void EditorLayer::CreateShadowMapPipeline()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.anisotropyEnable = VK_FALSE;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.maxAnisotropy = 1.0f;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 1.0f;
        sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        VK_CHECK_RESULT(vkCreateSampler(device, &sampler_info, nullptr, &m_ShadowMapSampler));

        m_ShadowMapUniformBuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

        auto bufferSize = sizeof(glm::mat4) * SHADOW_MAP_CASCADE_COUNT;

        BufferSpecification uniformBufferSpec;
        uniformBufferSpec.InstanceCount = 1;
        uniformBufferSpec.InstanceSize = bufferSize;
        uniformBufferSpec.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        uniformBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        for (auto& uniformBuffer : m_ShadowMapUniformBuffers)
        {
            uniformBuffer = std::make_unique<Buffer>(uniformBufferSpec);
            uniformBuffer->MapMemory(bufferSize);
        }

        // Creating Descriptors
        DescriptorSetLayoutSpecification shadowDescSetLayoutSpec;
        shadowDescSetLayoutSpec.Bindings.emplace_back();
        shadowDescSetLayoutSpec.Bindings[0].binding = 0;
        shadowDescSetLayoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        shadowDescSetLayoutSpec.Bindings[0].descriptorCount = 1;
        shadowDescSetLayoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        shadowDescSetLayoutSpec.Bindings[0].pImmutableSamplers = nullptr;

        m_ShadowMapDescriptorSetLayout = DescriptorSetLayout::Create(shadowDescSetLayoutSpec);

        m_ShadowMapDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

        DescriptorSetSpecification shadowMapDescSetSpec;
        shadowMapDescSetSpec.Layout = m_ShadowMapDescriptorSetLayout;

        for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_ShadowMapDescriptorSets[i] = DescriptorSet::Create(shadowMapDescSetSpec);

            VkDescriptorBufferInfo vk_descriptor_buffer_info{};
            vk_descriptor_buffer_info.buffer = m_ShadowMapUniformBuffers[i]->GetBuffer();
            vk_descriptor_buffer_info.offset = 0;
            vk_descriptor_buffer_info.range = bufferSize;

            m_ShadowMapDescriptorSets[i]->WriteBuffer(0, vk_descriptor_buffer_info);
            m_ShadowMapDescriptorSets[i]->Update();
        }

        VkPushConstantRange vk_push_constant_range{};
        vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        vk_push_constant_range.size = sizeof(ModelMatrixPushConstantData);
        vk_push_constant_range.offset = 0;

        VkDescriptorSetLayout descriptorSetLayouts[] = { m_ShadowMapDescriptorSetLayout->GetLayout() };
        VkPushConstantRange pushConstantRanges[] = { vk_push_constant_range };

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = 1;
        vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
        vk_pipeline_layout_create_info.pushConstantRangeCount = 1;
        vk_pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_ShadowMapPipelineLayout));

        PipelineSpecification pipelineSpec{};
        pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/shadow_map.vert.spv";
        pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/shadow_map.frag.spv";

        pipelineSpec.PipelineLayout = m_ShadowMapPipelineLayout;
        pipelineSpec.RenderPass = m_ShadowMapRenderPass;

        pipelineSpec.VertexLayout = { VertexInputAttribute::VEC3F };
        pipelineSpec.VertexInputBindingDescription = MeshVertex::GetBindingDescription();

        pipelineSpec.Viewport.width = SHADOW_MAP_SIZE;
        pipelineSpec.Viewport.height = SHADOW_MAP_SIZE;

        pipelineSpec.Scissor = { { 0, 0 }, { SHADOW_MAP_SIZE, SHADOW_MAP_SIZE } };

        // pipelineSpec.CullMode = VK_CULL_MODE_FRONT_BIT;
        pipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        pipelineSpec.DepthClampEnable = true;

        m_ShadowMapPipeline = Pipeline::Create(pipelineSpec);
    }

    void EditorLayer::CreateCompositePipeline()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        auto swapchain = VulkanContext::GetCurrentWindow()->GetSwapChain();
        VkFormat swapChainImageFormat = swapchain->GetSwapChainImageFormat();

        // Create render pass
        FramebufferSpecification framebufferSpec{};
        framebufferSpec.Width = m_ViewportSize.x;
        framebufferSpec.Height = m_ViewportSize.y;
        framebufferSpec.Attachments = { swapChainImageFormat, VK_FORMAT_D32_SFLOAT };
        framebufferSpec.ClearColorValue = { 0.0f, 0.0f, 0.0f, 1.0f };
        framebufferSpec.DepthStencilClearValue = { 1.0f, 0 };
        framebufferSpec.Samples = 1;

        RenderPassSpecification renderPassSpec{};
        renderPassSpec.TargetFramebuffers.resize(swapchain->GetSwapChainImageCount());
        for (uint32_t i = 0; i < renderPassSpec.TargetFramebuffers.size(); i++)
            renderPassSpec.TargetFramebuffers[i] = Framebuffer::Create(framebufferSpec);

        m_CompositePass = RenderPass::Create(renderPassSpec);

        DescriptorSetLayoutSpecification layoutSpec;
        layoutSpec.Bindings.resize(1);

        layoutSpec.Bindings[0].binding = 0;
        layoutSpec.Bindings[0].descriptorCount = 1;
        layoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // layoutSpec.Bindings[1].binding = 1;
        // layoutSpec.Bindings[1].descriptorCount = 1;
        // layoutSpec.Bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        // layoutSpec.Bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        m_CompositePassDescriptorSetLayout = DescriptorSetLayout::Create(layoutSpec);

        DescriptorSetSpecification setSpec;
        setSpec.Layout = m_CompositePassDescriptorSetLayout;

        auto imageCount = VulkanContext::GetCurrentWindow()->GetSwapChain()->GetSwapChainImageCount();

        m_CompositePassDescriptorSets.resize(imageCount);
        for (uint8_t i = 0; i < m_CompositePassDescriptorSets.size(); i++)
        {
            m_CompositePassDescriptorSets[i] = DescriptorSet::Create(setSpec);

            m_CompositePassDescriptorSets[i]->WriteImage(0, {
                 .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                 .imageView = m_SceneRenderPass->GetSpecification().TargetFramebuffers[i]->GetColorResolveAttachment(0)->GetImageView(),
                 .sampler = m_VkTextureSampler
                }
            );

            // m_CompositePassDescriptorSets[i]->WriteImage(1, {
            //     .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            //      .imageView = m_StencilImages[i]->GetImageView(),
            //      .sampler = m_ShadowMapSampler
            //     }
            // );

            m_CompositePassDescriptorSets[i]->Update();
        }

        // Pipeline Creation
        VkDescriptorSetLayout descriptorSetLayouts[] = { m_CompositePassDescriptorSetLayout->GetLayout() };
        VkPushConstantRange pushConstantRanges[] = {};

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = 1;
        vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
        vk_pipeline_layout_create_info.pushConstantRangeCount = 0;
        vk_pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_CompositePipelineLayout));

        PipelineSpecification pipelineSpec{};
        pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/composite.vert.spv";
        pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/composite.frag.spv";

        pipelineSpec.PipelineLayout = m_CompositePipelineLayout;
        pipelineSpec.RenderPass = m_CompositePass;

        pipelineSpec.VertexLayout = {};
        pipelineSpec.VertexInputBindingDescription.binding = 0;
        pipelineSpec.VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        pipelineSpec.VertexInputBindingDescription.stride = 0;

        pipelineSpec.BlendingEnable = true;
        pipelineSpec.CullMode = VK_CULL_MODE_FRONT_BIT;

        m_CompositePipeline = Pipeline::Create(pipelineSpec);
    }
}
