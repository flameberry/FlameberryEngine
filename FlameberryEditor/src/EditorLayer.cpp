#include "EditorLayer.h"

#include "ImGuizmo/ImGuizmo.h"
#include "Utils.h"

namespace Flameberry {
    EditorLayer::EditorLayer(const std::string_view& projectPath)
        : m_ProjectPath(projectPath)
    {}

    void EditorLayer::OnCreate()
    {
        m_VulkanRenderer = VulkanRenderer::Create((VulkanWindow*)&Application::Get().GetWindow());

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
        cameraInfo.zFar = 500.0f;
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
        vk_uniform_buffer_object_layout_binding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding shadowMapSamplerLayoutBinding{};
        shadowMapSamplerLayoutBinding.binding = 1;
        shadowMapSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shadowMapSamplerLayoutBinding.descriptorCount = 1;
        shadowMapSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        shadowMapSamplerLayoutBinding.pImmutableSamplers = nullptr;

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
            vk_shadow_map_image_info.imageView = m_VulkanRenderer->GetShadowMapImageView(i);
            vk_shadow_map_image_info.sampler = m_VulkanRenderer->GetShadowMapSampler(i);

            VulkanContext::GetCurrentGlobalDescriptorPool()->AllocateDescriptorSet(&m_VkDescriptorSets[i], m_VulkanDescriptorLayout->GetLayout());
            m_VulkanDescriptorWriter->WriteBuffer(0, &vk_descriptor_buffer_info);
            m_VulkanDescriptorWriter->WriteImage(1, &vk_shadow_map_image_info);
            m_VulkanDescriptorWriter->Update(m_VkDescriptorSets[i]);
        }

        // m_SkyboxRenderer = std::make_unique<SkyboxRenderer>(m_VulkanRenderer->GetGlobalDescriptorPool(), m_VulkanDescriptorLayout->GetLayout(), m_VulkanRenderer->GetRenderPass());
        m_SceneRenderer = std::make_unique<SceneRenderer>(VulkanContext::GetCurrentGlobalDescriptorPool(), m_VulkanDescriptorLayout->GetLayout(), m_VulkanRenderer->GetRenderPass());
        m_ImGuiLayer = std::make_unique<ImGuiLayer>(m_VulkanRenderer);

        // Test
        m_VkTextureSampler = VulkanRenderCommand::CreateDefaultSampler();

        m_ViewportDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_ViewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                m_VkTextureSampler,
                m_VulkanRenderer->GetViewportImageView(i),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
        }

        m_ShadowMapDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_ShadowMapDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                m_VkTextureSampler,
                m_VulkanRenderer->GetShadowMapImageView(i),
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            );
        }

        m_Registry = std::make_shared<ecs::registry>();
        m_ActiveScene = std::make_shared<Scene>(m_Registry.get());

        m_SceneHierarchyPanel = std::make_shared<SceneHierarchyPanel>(m_ActiveScene.get());
        m_ContentBrowserPanel = std::make_shared<ContentBrowserPanel>(m_ProjectPath);

        // std::shared_ptr<Material> material = std::make_shared<Material>();
        // material->Name = "Test Material";
        // material->TextureMapEnabled = true;
        // material->TextureMap = VulkanTexture::TryGetOrLoadTexture("Assets/Textures/brick.png");
        // MaterialSerializer::Serialize(material, "Assets/Materials/Test.fbmat");

        // auto mat = MaterialSerializer::Deserialize("Assets/Materials/Test.fbmat");

        // FL_DEBUGBREAK();
    }

    void EditorLayer::OnUpdate(float delta)
    {
        FL_PROFILE_SCOPE("Editor_OnUpdate");

        if (VkCommandBuffer commandBuffer = m_VulkanRenderer->BeginFrame())
        {
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

                m_VulkanRenderer->BeginShadowRenderPass(lightViewProjectionMatrix);
                m_SceneRenderer->OnDrawForShadowPass(commandBuffer, m_VulkanRenderer->GetShadowMapPipelineLayout(), m_ActiveCamera, m_ActiveScene);
                m_VulkanRenderer->EndShadowRenderPass();
            }

            m_VulkanRenderer->UpdateViewportSize(m_ViewportSize);

            {
                FL_PROFILE_SCOPE("Viewport Render Pass");
                m_VulkanRenderer->BeginViewportRenderPass(m_ActiveScene->GetClearColor());

                VulkanRenderCommand::SetViewport(commandBuffer, 0.0f, 0.0f, m_ViewportSize.x, m_ViewportSize.y);
                VulkanRenderCommand::SetScissor(commandBuffer, { 0, 0 }, VkExtent2D{ (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y });

                // Update Uniforms
                m_ActiveCamera.OnResize(m_ViewportSize.x / m_ViewportSize.y);

                if (m_IsViewportFocused)
                    m_IsCameraMoving = m_ActiveCamera.OnUpdate(delta);

                CameraUniformBufferObject uniformBufferObject{};
                uniformBufferObject.ViewProjectionMatrix = m_ActiveCamera.GetViewProjectionMatrix();
                uniformBufferObject.LightViewProjectionMatrix = lightViewProjectionMatrix;

                m_UniformBuffers[m_VulkanRenderer->GetCurrentFrameIndex()]->WriteToBuffer(&uniformBufferObject, sizeof(uniformBufferObject), 0);

                // m_SkyboxRenderer->OnDraw(commandBuffer, m_VulkanRenderer->GetCurrentFrameIndex(), m_VkDescriptorSets[m_VulkanRenderer->GetCurrentFrameIndex()], m_ActiveCamera, "");
                m_SceneRenderer->OnDraw(commandBuffer, m_VulkanRenderer->GetCurrentFrameIndex(), m_VkDescriptorSets[m_VulkanRenderer->GetCurrentFrameIndex()], m_ActiveCamera, m_ActiveScene);

                m_VulkanRenderer->EndViewportRenderPass();
            }

            m_ImGuiLayer->Begin();
            OnUIRender();
            m_ImGuiLayer->End(commandBuffer, m_VulkanRenderer->GetCurrentFrameIndex(), m_VulkanRenderer->GetSwapChainExtent2D());

            bool isResized = m_VulkanRenderer->EndFrame();
            if (isResized)
                m_ImGuiLayer->InvalidateResources(m_VulkanRenderer);
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
        vkDestroySampler(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), m_VkTextureSampler, nullptr);
        m_ImGuiLayer->OnDestroy();
    }

    void EditorLayer::OnUIRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        bool beginViewport = ImGui::Begin("Viewport");

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
                        m_SceneHierarchyPanel->SetSelectedEntity(entity);
                    }
                }
                else
                    FL_WARN("Bad File given as Scene!");
            }
            ImGui::EndDragDropTarget();
        }

        // ImGuizmo
        const auto& selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity();
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

        if (beginViewport) {
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
                m_ShadowMapDescriptorSets[currentFrameIndex]),
                ImVec2{ m_ShadowMapViewportSize.x, m_ShadowMapViewportSize.y }
            );
            ImGui::End();
        }

        m_SceneHierarchyPanel->OnUIRender();
        m_ContentBrowserPanel->OnUIRender();
    }

    void EditorLayer::InvalidateViewportImGuiDescriptorSet()
    {
        uint32_t currentFrameIndex = m_VulkanRenderer->GetCurrentFrameIndex();

        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = m_VkTextureSampler;
        desc_image[0].imageView = m_VulkanRenderer->GetViewportImageView(currentFrameIndex);
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
        desc_image[0].imageView = m_VulkanRenderer->GetShadowMapImageView(currentFrameIndex);
        desc_image[0].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write_desc[1] = {};
        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = m_ShadowMapDescriptorSets[currentFrameIndex];
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
}
