#include "EditorLayer.h"

#include "ImGuizmo/ImGuizmo.h"
#include "Utils.h"

namespace Flameberry {
    EditorLayer::EditorLayer(const std::string_view& projectPath)
        : m_ProjectPath(projectPath)
    {}

    void EditorLayer::OnCreate()
    {
        {
            FramebufferSpecification shadowMapFramebufferSpec;
            shadowMapFramebufferSpec.Width = 2048;
            shadowMapFramebufferSpec.Height = 2048;
            shadowMapFramebufferSpec.Attachments = { VK_FORMAT_D32_SFLOAT };
            shadowMapFramebufferSpec.Samples = 1;
            shadowMapFramebufferSpec.DepthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            shadowMapFramebufferSpec.DepthStencilClearValue = { 1.0f, 0 };

            RenderPassSpecification shadowMapRenderPassSpec;

            uint32_t imageCount = 3;
            m_ShadowMapFramebuffers.resize(imageCount);
            shadowMapRenderPassSpec.TargetFramebuffers.resize(imageCount);

            for (uint32_t i = 0; i < imageCount; i++)
            {
                m_ShadowMapFramebuffers[i] = Framebuffer::Create(shadowMapFramebufferSpec);
                shadowMapRenderPassSpec.TargetFramebuffers[i] = m_ShadowMapFramebuffers[i];
            }

            m_ShadowMapRenderPass = RenderPass::Create(shadowMapRenderPassSpec);
        }
        CreateShadowMapPipeline();

        m_VulkanRenderer = VulkanRenderer::Create((VulkanWindow*)&Application::Get().GetWindow(), m_ShadowMapRenderPass);

        m_CursorIcon = VulkanTexture::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/cursor_icon.png");
        m_CursorIconActive = VulkanTexture::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/cursor_icon_active.png");
        m_TranslateIcon = VulkanTexture::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/translate_icon_2.png");
        m_TranslateIconActive = VulkanTexture::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/translate_icon_2_active.png");
        m_RotateIcon = VulkanTexture::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/rotate_icon.png");
        m_RotateIconActive = VulkanTexture::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/rotate_icon_active.png");
        m_ScaleIcon = VulkanTexture::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/scale_icon.png");
        m_ScaleIconActive = VulkanTexture::TryGetOrLoadTexture(FL_PROJECT_DIR"FlameberryEditor/assets/icons/scale_icon_active.png");

        PerspectiveCameraInfo cameraInfo{};
        cameraInfo.aspectRatio = Application::Get().GetWindow().GetWidth() / Application::Get().GetWindow().GetHeight();
        cameraInfo.FOV = 45.0f;
        cameraInfo.zNear = 0.1f;
        cameraInfo.zFar = 1000.0f;
        cameraInfo.cameraPostion = glm::vec3(0.0f, 0.0f, 4.0f);
        cameraInfo.cameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);

        m_ActiveCamera = PerspectiveCamera(cameraInfo);

        // Creating Uniform Buffers
        VkDeviceSize uniformBufferSize = sizeof(CameraUniformBufferObject);
        for (auto& uniformBuffer : m_UniformBuffers)
        {
            uniformBuffer = std::make_unique<VulkanBuffer>(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            uniformBuffer->MapMemory(uniformBufferSize);
        }

        // Creating Descriptors
        VkDescriptorSetLayoutBinding vk_uniform_buffer_object_layout_binding{};
        vk_uniform_buffer_object_layout_binding.binding = 0;
        vk_uniform_buffer_object_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        vk_uniform_buffer_object_layout_binding.descriptorCount = 1;
        vk_uniform_buffer_object_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding shadowMapSamplerLayoutBinding{};
        shadowMapSamplerLayoutBinding.binding = 1;
        shadowMapSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shadowMapSamplerLayoutBinding.descriptorCount = 1;
        shadowMapSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { vk_uniform_buffer_object_layout_binding, shadowMapSamplerLayoutBinding };

        m_VulkanDescriptorLayout = std::make_unique<VulkanDescriptorLayout>(bindings);
        m_VulkanDescriptorWriter = std::make_unique<VulkanDescriptorWriter>(*m_VulkanDescriptorLayout);

        m_VkDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

        for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo vk_descriptor_buffer_info{};
            vk_descriptor_buffer_info.buffer = m_UniformBuffers[i]->GetBuffer();
            vk_descriptor_buffer_info.offset = 0;
            vk_descriptor_buffer_info.range = sizeof(CameraUniformBufferObject);

            VkDescriptorImageInfo vk_shadow_map_image_info{};
            vk_shadow_map_image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            vk_shadow_map_image_info.imageView = m_ShadowMapFramebuffers[i]->GetAttachmentImageView(0);
            vk_shadow_map_image_info.sampler = m_ShadowMapSampler;

            VulkanContext::GetCurrentGlobalDescriptorPool()->AllocateDescriptorSet(&m_VkDescriptorSets[i], m_VulkanDescriptorLayout->GetLayout());
            m_VulkanDescriptorWriter->WriteBuffer(0, &vk_descriptor_buffer_info);
            m_VulkanDescriptorWriter->WriteImage(1, &vk_shadow_map_image_info);
            m_VulkanDescriptorWriter->Update(m_VkDescriptorSets[i]);
        }

        // m_SkyboxRenderer = std::make_unique<SkyboxRenderer>(m_VulkanRenderer->GetGlobalDescriptorPool(), m_VulkanDescriptorLayout->GetLayout(), m_VulkanRenderer->GetRenderPass());
        m_ImGuiLayer = std::make_unique<ImGuiLayer>(m_VulkanRenderer);

        m_Registry = std::make_shared<ecs::registry>();
        m_ActiveScene = Scene::Create(m_Registry);

        m_SceneHierarchyPanel = SceneHierarchyPanel::Create(m_ActiveScene);
        m_ContentBrowserPanel = ContentBrowserPanel::Create(m_ProjectPath);
        m_EnvironmentSettingsPanel = EnvironmentSettingsPanel::Create(m_ActiveScene);

        m_MousePickingBuffer = std::make_unique<VulkanBuffer>(
            sizeof(int32_t),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        {
            VkFormat swapChainImageFormat = m_VulkanRenderer->GetSwapChainImageFormat();
            VkSampleCountFlagBits sampleCount = VulkanRenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

            FramebufferSpecification sceneFramebufferSpec;
            sceneFramebufferSpec.Width = m_ViewportSize.x;
            sceneFramebufferSpec.Height = m_ViewportSize.y;
            sceneFramebufferSpec.Attachments = { swapChainImageFormat, VK_FORMAT_D32_SFLOAT };
            sceneFramebufferSpec.Samples = sampleCount;
            sceneFramebufferSpec.ClearColorValue = { 0.0f, 0.0f, 0.0f, 1.0f };
            sceneFramebufferSpec.DepthStencilClearValue = { 1.0f, 0 };

            RenderPassSpecification sceneRenderPassSpec;

            uint32_t imageCount = m_VulkanRenderer->GetSwapChainImages().size();
            m_SceneFramebuffers.resize(imageCount);
            sceneRenderPassSpec.TargetFramebuffers.resize(imageCount);

            for (uint32_t i = 0; i < imageCount; i++)
            {
                m_SceneFramebuffers[i] = Framebuffer::Create(sceneFramebufferSpec);
                sceneRenderPassSpec.TargetFramebuffers[i] = m_SceneFramebuffers[i];
            }

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

            m_MousePickingFramebuffer = Framebuffer::Create(mousePickingFramebufferSpec);

            RenderPassSpecification mousePickingRenderPassSpec;
            mousePickingRenderPassSpec.TargetFramebuffers = { m_MousePickingFramebuffer };

            m_MousePickingRenderPass = RenderPass::Create(mousePickingRenderPassSpec);
        }

        CreateMousePickingPipeline();

        m_SceneRenderer = std::make_unique<SceneRenderer>(m_VulkanDescriptorLayout->GetLayout(), m_SceneRenderPass);

        m_VkTextureSampler = VulkanRenderCommand::CreateDefaultSampler();
        m_ViewportDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_ViewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                m_VkTextureSampler,
                m_SceneFramebuffers[i]->GetAttachmentImageView(2),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
        }

        m_ShadowMapViewportDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_ShadowMapViewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                m_VkTextureSampler,
                m_ShadowMapFramebuffers[i]->GetAttachmentImageView(0),
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            );
        }
    }

    void EditorLayer::OnUpdate(float delta)
    {
        FL_PROFILE_SCOPE("Editor_OnUpdate");

        bool isClickedInsideViewport = false;
        int mouseX = 0;
        int mouseY = 0;

        if (VkCommandBuffer commandBuffer = m_VulkanRenderer->BeginFrame())
        {
            uint32_t currentFrameIndex = m_VulkanRenderer->GetCurrentFrameIndex();

            glm::mat4 lightViewProjectionMatrix;
            {
                FL_PROFILE_SCOPE("Shadow Pass");
                glm::mat4 lightProjectionMatrix = glm::ortho<float>(-40, 40, -40, 40, m_ZNearFar.x, m_ZNearFar.y);
                glm::mat4 lightViewMatrix = glm::lookAt(
                    -m_ActiveScene->GetDirectionalLight().Direction,
                    glm::vec3(0, 0, 0),
                    glm::vec3(0, 1, 0)
                );
                lightViewProjectionMatrix = lightProjectionMatrix * lightViewMatrix;

                m_ShadowMapRenderPass->Begin(commandBuffer, m_VulkanRenderer->GetImageIndex());

                m_ShadowMapPipeline->Bind(commandBuffer);
                m_ShadowMapUniformBuffers[currentFrameIndex]->WriteToBuffer(glm::value_ptr(lightViewProjectionMatrix), sizeof(glm::mat4), 0);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ShadowMapPipelineLayout, 0, 1, &m_ShadowMapDescriptorSets[currentFrameIndex], 0, nullptr);

                m_SceneRenderer->OnDrawForShadowPass(commandBuffer, m_ShadowMapPipelineLayout, m_ActiveCamera, m_ActiveScene);
                m_ShadowMapRenderPass->End(commandBuffer);
            }

            m_SceneFramebuffers[m_VulkanRenderer->GetImageIndex()]->Resize(m_ViewportSize.x, m_ViewportSize.y, m_SceneRenderPass->GetRenderPass());
            m_SceneFramebuffers[m_VulkanRenderer->GetImageIndex()]->SetClearColorValue({ m_ActiveScene->GetClearColor().x, m_ActiveScene->GetClearColor().y, m_ActiveScene->GetClearColor().z, 1.0f });

            {
                FL_PROFILE_SCOPE("Viewport Render Pass");
                m_SceneRenderPass->Begin(commandBuffer, m_VulkanRenderer->GetImageIndex());

                VulkanRenderCommand::SetViewport(commandBuffer, 0.0f, 0.0f, m_ViewportSize.x, m_ViewportSize.y);
                VulkanRenderCommand::SetScissor(commandBuffer, { 0, 0 }, VkExtent2D{ (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y });

                // Update Uniforms
                m_ActiveCamera.OnResize(m_ViewportSize.x / m_ViewportSize.y);

                if (m_IsViewportFocused)
                    m_IsCameraMoving = m_ActiveCamera.OnUpdate(delta);

                CameraUniformBufferObject uniformBufferObject{};
                uniformBufferObject.ViewProjectionMatrix = m_ActiveCamera.GetViewProjectionMatrix();
                uniformBufferObject.LightViewProjectionMatrix = lightViewProjectionMatrix;

                m_UniformBuffers[currentFrameIndex]->WriteToBuffer(&uniformBufferObject, sizeof(uniformBufferObject), 0);

                // m_SkyboxRenderer->OnDraw(commandBuffer, currentFrameIndex, m_VkDescriptorSets[currentFrameIndex], m_ActiveCamera, "");
                m_SceneRenderer->OnDraw(commandBuffer, currentFrameIndex, m_VkDescriptorSets[currentFrameIndex], m_ActiveCamera, m_ActiveScene);

                m_SceneRenderPass->End(commandBuffer);
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
                mouseX = (int)mx;
                mouseY = (int)my;

                if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
                {
                    FL_PROFILE_SCOPE("Last Mouse Picking Pass");
                    isClickedInsideViewport = true;

                    m_MousePickingRenderPass->Begin(commandBuffer, 0, { mouseX, (int)(m_ViewportSize.y - mouseY) }, { 1, 1 });

                    const auto& device = VulkanContext::GetCurrentDevice();
                    m_MousePickingPipeline->Bind(commandBuffer);
                    m_MousePickingUniformBuffer->WriteToBuffer(glm::value_ptr(m_ActiveCamera.GetViewProjectionMatrix()), sizeof(glm::mat4), 0);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MousePickingPipelineLayout, 0, 1, &m_MousePickingDescriptorSet, 0, nullptr);

                    m_SceneRenderer->OnDrawForMousePickingPass(commandBuffer, m_MousePickingPipelineLayout, m_ActiveCamera, m_ActiveScene);
                    m_MousePickingRenderPass->End(commandBuffer);
                }
            }

            m_ImGuiLayer->Begin();
            OnUIRender();
            m_ImGuiLayer->End(commandBuffer, currentFrameIndex, m_VulkanRenderer->GetSwapChainExtent2D());

            bool isResized = m_VulkanRenderer->EndFrame();
            if (isResized)
                m_ImGuiLayer->InvalidateResources(m_VulkanRenderer);
        }

        if (isClickedInsideViewport)
        {
            m_VulkanRenderer->WriteMousePickingImagePixelToBuffer(m_MousePickingBuffer->GetBuffer(), m_MousePickingFramebuffer->GetAttachmentImage(0), { mouseX, m_ViewportSize.y - mouseY });
            m_MousePickingBuffer->MapMemory(sizeof(int32_t));
            int32_t* data = (int32_t*)m_MousePickingBuffer->GetMappedMemory();
            int32_t entityID = data[0];
            m_MousePickingBuffer->UnmapMemory();
            // FL_LOG("Selected Entity: {0}", entityID);
            m_SceneHierarchyPanel->SetSelectionContext((entityID != -1) ? ecs::entity_handle(entityID) : ecs::entity_handle::null);
        }

        if (m_ShouldOpenAnotherScene)
        {
            VulkanContext::GetCurrentDevice()->WaitIdleGraphicsQueue();
            OpenScene(m_ScenePathToBeOpened);
            m_ShouldOpenAnotherScene = false;
            m_ScenePathToBeOpened = "";
        }
    }

    void EditorLayer::OnDestroy()
    {
        VulkanContext::GetCurrentDevice()->WaitIdle();
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroySampler(device, m_VkTextureSampler, nullptr);
        vkDestroySampler(device, m_ShadowMapSampler, nullptr);
        vkDestroyPipelineLayout(device, m_ShadowMapPipelineLayout, nullptr);
        vkDestroyPipelineLayout(device, m_MousePickingPipelineLayout, nullptr);
        m_ImGuiLayer->OnDestroy();
    }

    void EditorLayer::OnUIRender()
    {
        // ShowMenuBar();

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
        InvalidateViewportImGuiDescriptorSet();

        uint32_t currentFrameIndex = m_VulkanRenderer->GetCurrentFrameIndex();
        ImGui::Image(reinterpret_cast<ImTextureID>(
            m_ViewportDescriptorSets[currentFrameIndex]),
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
                        const auto& staticMesh = AssetManager::TryGetOrLoadAssetFromFile<StaticMesh>(path);
                        auto entity = m_Registry->create();
                        m_Registry->emplace<IDComponent>(entity);
                        m_Registry->emplace<TagComponent>(entity, "StaticMesh");
                        m_Registry->emplace<TransformComponent>(entity);
                        m_Registry->emplace<MeshComponent>(entity, staticMesh->GetUUID());
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
        if (selectedEntity != ecs::entity_handle::null && m_GizmoType != -1)
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            float windowWidth = (float)ImGui::GetWindowWidth();
            float windowHeight = (float)ImGui::GetWindowHeight();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

            glm::mat4 projectionMatrix = m_ActiveCamera.GetProjectionMatrix();
            projectionMatrix[1][1] *= -1;
            const auto& viewMatrix = m_ActiveCamera.GetViewMatrix();

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

        ImGui::Begin("Statistics");
        ImGui::TextWrapped("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        FL_DISPLAY_SCOPE_DETAILS_IMGUI();
        ImGui::Separator();
        ImGui::End();

        ImGui::Begin("Settings");
        ImGui::DragFloat2("zNear, zFar", glm::value_ptr(m_ZNearFar), 0.2f);
        ImGui::Checkbox("Display Shadow Map", &m_DisplayShadowMap);
        ImGui::End();

        if (m_DisplayShadowMap) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
            ImGui::Begin("Shadow Map");
            ImGui::PopStyleVar();

            ImVec2 shadowMapViewportSize = ImGui::GetContentRegionAvail();
            m_ShadowMapViewportSize = { shadowMapViewportSize.x, shadowMapViewportSize.y };

            InvalidateShadowMapImGuiDescriptorSet();

            ImGui::Image(reinterpret_cast<ImTextureID>(
                m_ShadowMapViewportDescriptorSets[currentFrameIndex]),
                ImVec2{ m_ShadowMapViewportSize.x, m_ShadowMapViewportSize.y }
            );
            ImGui::End();
        }

        m_SceneHierarchyPanel->OnUIRender();
        m_ContentBrowserPanel->OnUIRender();
        m_EnvironmentSettingsPanel->OnUIRender();
    }

    void EditorLayer::InvalidateViewportImGuiDescriptorSet()
    {
        uint32_t currentFrameIndex = m_VulkanRenderer->GetCurrentFrameIndex();

        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = m_VkTextureSampler;
        desc_image[0].imageView = m_SceneFramebuffers[m_VulkanRenderer->GetImageIndex()]->GetAttachmentImageView(2);
        desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write_desc[1] = {};
        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = m_ViewportDescriptorSets[currentFrameIndex];
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc[0].pImageInfo = desc_image;
        vkUpdateDescriptorSets(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), 1, write_desc, 0, nullptr);
    }

    void EditorLayer::InvalidateShadowMapImGuiDescriptorSet()
    {
        uint32_t currentFrameIndex = m_VulkanRenderer->GetCurrentFrameIndex();

        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = m_VkTextureSampler;
        desc_image[0].imageView = m_ShadowMapFramebuffers[m_VulkanRenderer->GetImageIndex()]->GetAttachmentImageView(0);
        desc_image[0].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write_desc[1] = {};
        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = m_ShadowMapViewportDescriptorSets[currentFrameIndex];
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc[0].pImageInfo = desc_image;
        vkUpdateDescriptorSets(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), 1, write_desc, 0, nullptr);
    }

    void EditorLayer::OnEvent(Event& e)
    {
        m_ImGuiLayer->OnEvent(e);
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
            if (ctrl_or_cmd) {
                m_ShouldOpenAnotherScene = true;
                m_ScenePathToBeOpened = platform::OpenDialog();
            }
            break;
        case GLFW_KEY_S:
            if (ctrl_or_cmd)
            {
                if (shift || m_OpenedScenePathIfExists.empty())
                    SaveScene();
                else
                    SaveScene(m_OpenedScenePathIfExists);
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
        std::string savePath = platform::SaveDialog();
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
        std::string sceneToBeLoaded = platform::OpenDialog();
        if (sceneToBeLoaded != "")
        {
            SceneSerializer serializer(m_ActiveScene);
            bool success = serializer.DeserializeScene(sceneToBeLoaded.c_str());
            if (success)
            {
                m_OpenedScenePathIfExists = sceneToBeLoaded;
                FL_INFO("Loaded Scene: {0}", sceneToBeLoaded);
                return;
            }
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
            bool success = serializer.DeserializeScene(path.c_str());
            if (success)
                m_OpenedScenePathIfExists = path;
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
                    SaveScene();
                if (ImGui::MenuItem("Save", "Cmd+S"))
                {
                    if (m_OpenedScenePathIfExists.empty())
                        SaveScene();
                    else
                        SaveScene(m_OpenedScenePathIfExists);
                }
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

        // Create Descriptors
        m_MousePickingUniformBuffer = std::make_unique<VulkanBuffer>(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_MousePickingUniformBuffer->MapMemory(sizeof(glm::mat4));

        // Creating Descriptors
        VkDescriptorSetLayoutBinding vk_uniform_buffer_object_layout_binding{};
        vk_uniform_buffer_object_layout_binding.binding = 0;
        vk_uniform_buffer_object_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        vk_uniform_buffer_object_layout_binding.descriptorCount = 1;
        vk_uniform_buffer_object_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        vk_uniform_buffer_object_layout_binding.pImmutableSamplers = nullptr;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { vk_uniform_buffer_object_layout_binding };

        m_MousePickingDescriptorLayout = std::make_unique<VulkanDescriptorLayout>(bindings);
        m_MousePickingDescriptorWriter = std::make_unique<VulkanDescriptorWriter>(*m_MousePickingDescriptorLayout);

        VkDescriptorBufferInfo vk_descriptor_buffer_info{};
        vk_descriptor_buffer_info.buffer = m_MousePickingUniformBuffer->GetBuffer();
        vk_descriptor_buffer_info.offset = 0;
        vk_descriptor_buffer_info.range = sizeof(glm::mat4);

        VulkanContext::GetCurrentGlobalDescriptorPool()->AllocateDescriptorSet(&m_MousePickingDescriptorSet, m_MousePickingDescriptorLayout->GetLayout());
        m_MousePickingDescriptorWriter->WriteBuffer(0, &vk_descriptor_buffer_info);
        m_MousePickingDescriptorWriter->Update(m_MousePickingDescriptorSet);

        // Pipeline Creation
        VkPushConstantRange vk_push_constant_range{};
        vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        vk_push_constant_range.size = sizeof(MousePickingPushConstantData);
        vk_push_constant_range.offset = 0;

        VkDescriptorSetLayout descriptorSetLayouts[] = { m_MousePickingDescriptorLayout->GetLayout() };
        VkPushConstantRange pushConstantRanges[] = { vk_push_constant_range };

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = 1;
        vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
        vk_pipeline_layout_create_info.pushConstantRangeCount = 1;
        vk_pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_MousePickingPipelineLayout));

        PipelineSpecification pipelineSpec{};
        pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/mouse_pickingVert.spv";
        pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/mouse_pickingFrag.spv";

        pipelineSpec.PipelineLayout = m_MousePickingPipelineLayout;
        pipelineSpec.RenderPass = m_MousePickingRenderPass;

        pipelineSpec.VertexLayout = { VertexInputAttribute::VEC3F };
        pipelineSpec.VertexInputBindingDescription = VulkanVertex::GetBindingDescription();

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
        sampler_info.anisotropyEnable = VK_TRUE;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.maxAnisotropy = 1.0f;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 1.0f;
        sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(VulkanContext::GetPhysicalDevice(), &properties);

        sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 1.0f;

        VK_CHECK_RESULT(vkCreateSampler(device, &sampler_info, nullptr, &m_ShadowMapSampler));

        m_ShadowMapUniformBuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (auto& uniformBuffer : m_ShadowMapUniformBuffers)
        {
            uniformBuffer = std::make_unique<VulkanBuffer>(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            uniformBuffer->MapMemory(sizeof(glm::mat4));
        }

        // Creating Descriptors
        VkDescriptorSetLayoutBinding vk_uniform_buffer_object_layout_binding{};
        vk_uniform_buffer_object_layout_binding.binding = 0;
        vk_uniform_buffer_object_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        vk_uniform_buffer_object_layout_binding.descriptorCount = 1;
        vk_uniform_buffer_object_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        vk_uniform_buffer_object_layout_binding.pImmutableSamplers = nullptr;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { vk_uniform_buffer_object_layout_binding };

        m_ShadowMapDescriptorLayout = std::make_unique<VulkanDescriptorLayout>(bindings);
        m_ShadowMapDescriptorWriter = std::make_unique<VulkanDescriptorWriter>(*m_ShadowMapDescriptorLayout);

        m_ShadowMapDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

        for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo vk_descriptor_buffer_info{};
            vk_descriptor_buffer_info.buffer = m_ShadowMapUniformBuffers[i]->GetBuffer();
            vk_descriptor_buffer_info.offset = 0;
            vk_descriptor_buffer_info.range = sizeof(glm::mat4);

            VulkanContext::GetCurrentGlobalDescriptorPool()->AllocateDescriptorSet(&m_ShadowMapDescriptorSets[i], m_ShadowMapDescriptorLayout->GetLayout());
            m_ShadowMapDescriptorWriter->WriteBuffer(0, &vk_descriptor_buffer_info);
            m_ShadowMapDescriptorWriter->Update(m_ShadowMapDescriptorSets[i]);
        }

        VkPushConstantRange vk_push_constant_range{};
        vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        vk_push_constant_range.size = sizeof(ModelMatrixPushConstantData);
        vk_push_constant_range.offset = 0;

        VkDescriptorSetLayout descriptorSetLayouts[] = { m_ShadowMapDescriptorLayout->GetLayout() };
        VkPushConstantRange pushConstantRanges[] = { vk_push_constant_range };

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = 1;
        vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
        vk_pipeline_layout_create_info.pushConstantRangeCount = 1;
        vk_pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_ShadowMapPipelineLayout));

        PipelineSpecification pipelineSpec{};
        pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/shadow_mapVert.spv";
        pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/shadow_mapFrag.spv";

        pipelineSpec.PipelineLayout = m_ShadowMapPipelineLayout;
        pipelineSpec.RenderPass = m_ShadowMapRenderPass;

        pipelineSpec.VertexLayout = { VertexInputAttribute::VEC3F };
        pipelineSpec.VertexInputBindingDescription = VulkanVertex::GetBindingDescription();

        pipelineSpec.Viewport.width = SHADOW_MAP_WIDTH;
        pipelineSpec.Viewport.height = SHADOW_MAP_HEIGHT;

        pipelineSpec.Scissor = { {0, 0}, {SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT} };

        m_ShadowMapPipeline = Pipeline::Create(pipelineSpec);
    }
}
