#include "FlameberryEditor.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

namespace Flameberry {
    FlameberryEditor::FlameberryEditor()
    {
        m_VulkanRenderer = VulkanRenderer::Create((VulkanWindow*)&Application::Get().GetWindow());

        PerspectiveCameraInfo cameraInfo{};
        cameraInfo.aspectRatio = Application::Get().GetWindow().GetWidth() / Application::Get().GetWindow().GetHeight();
        cameraInfo.FOV = 45.0f;
        cameraInfo.zNear = 0.1f;
        cameraInfo.zFar = 500.0f;
        cameraInfo.cameraPostion = glm::vec3(0.0f, 0.0f, 4.0f);
        cameraInfo.cameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);

        m_ActiveCamera = PerspectiveCamera(cameraInfo);

        m_Texture = std::make_unique<VulkanTexture>(FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png");

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

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding shadowMapSamplerLayoutBinding{};
        shadowMapSamplerLayoutBinding.binding = 2;
        shadowMapSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shadowMapSamplerLayoutBinding.descriptorCount = 1;
        shadowMapSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        shadowMapSamplerLayoutBinding.pImmutableSamplers = nullptr;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { vk_uniform_buffer_object_layout_binding, samplerLayoutBinding, shadowMapSamplerLayoutBinding };

        m_VulkanDescriptorLayout = std::make_unique<VulkanDescriptorLayout>(bindings);
        m_VulkanDescriptorWriter = std::make_unique<VulkanDescriptorWriter>(*m_VulkanDescriptorLayout);

        m_VkDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

        for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo vk_descriptor_buffer_info{};
            vk_descriptor_buffer_info.buffer = m_UniformBuffers[i]->GetBuffer();
            vk_descriptor_buffer_info.offset = 0;
            vk_descriptor_buffer_info.range = sizeof(CameraUniformBufferObject);

            VkDescriptorImageInfo vk_image_info{};
            vk_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            vk_image_info.imageView = m_Texture->GetImageView();
            vk_image_info.sampler = m_Texture->GetSampler();

            VkDescriptorImageInfo vk_shadow_map_image_info{};
            vk_shadow_map_image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            vk_shadow_map_image_info.imageView = m_VulkanRenderer->GetShadowMapImageView(i);
            vk_shadow_map_image_info.sampler = m_VulkanRenderer->GetShadowMapSampler(i);

            m_VulkanRenderer->GetGlobalDescriptorPool()->AllocateDescriptorSet(&m_VkDescriptorSets[i], m_VulkanDescriptorLayout->GetLayout());
            m_VulkanDescriptorWriter->WriteBuffer(0, &vk_descriptor_buffer_info);
            m_VulkanDescriptorWriter->WriteImage(1, &vk_image_info);
            m_VulkanDescriptorWriter->WriteImage(2, &vk_shadow_map_image_info);
            m_VulkanDescriptorWriter->Update(m_VkDescriptorSets[i]);
        }

        m_MeshRenderer = std::make_unique<MeshRenderer>(m_VulkanRenderer->GetGlobalDescriptorPool(), m_VulkanDescriptorLayout->GetLayout(), m_VulkanRenderer->GetRenderPass());

        {
            auto [vk_vertices, indices] = VulkanRenderCommand::LoadModel(FL_PROJECT_DIR"SandboxApp/assets/models/cube.obj");
            m_Meshes.emplace_back(VulkanMesh::Create(vk_vertices, indices));
        }
        {
            auto [vk_vertices, indices] = VulkanRenderCommand::LoadModel(FL_PROJECT_DIR"SandboxApp/assets/models/sponza.obj");
            m_Meshes.emplace_back(VulkanMesh::Create(vk_vertices, indices));
        }
        // {
        //     auto [vk_vertices, indices] = VulkanRenderCommand::LoadModel(FL_PROJECT_DIR"SandboxApp/assets/models/plane.obj");
        //     m_Meshes.emplace_back(VulkanMesh::Create(vk_vertices, indices));
        // }

        m_ImGuiLayer = std::make_unique<ImGuiLayer>();
        m_ImGuiLayer->OnAttach(m_VulkanRenderer);

        // Test
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &m_VkTextureSampler));

        m_ViewportDescriptorSets.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_ViewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                m_VkTextureSampler,
                m_VulkanRenderer->GetViewportImageView(i),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
        }
    }

    void FlameberryEditor::OnUpdate(float delta)
    {
        // FL_SCOPED_TIMER("FlameberryEditor::OnUpdate");
        FL_PROFILE_SCOPE("Vulkan Frame Render Time");
        m_ActiveCamera.OnUpdate(delta);
        if (VkCommandBuffer commandBuffer = m_VulkanRenderer->BeginFrame())
        {
            glm::mat4 lightViewProjectionMatrix;
            if (m_VulkanRenderer->EnableShadows())
            {
                FL_PROFILE_SCOPE("Shadow Pass");
                // glm::vec3 lightInvDir(1.0f, 3.0f, 2.0f);
                glm::vec3 lightInvDir(5.0f);

                glm::mat4 lightProjectionMatrix = glm::ortho<float>(-10, 10, -10, 10, -10, 20);
                glm::mat4 lightViewMatrix = glm::lookAt(lightInvDir, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
                lightViewProjectionMatrix = lightProjectionMatrix * lightViewMatrix;

                m_VulkanRenderer->BeginShadowRenderPass(lightViewProjectionMatrix);

                for (auto& mesh : m_Meshes)
                {
                    ModelMatrixPushConstantData pushContantData;
                    pushContantData.ModelMatrix = glm::mat4(1.0f);
                    vkCmdPushConstants(commandBuffer, m_VulkanRenderer->GetShadowMapPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrixPushConstantData), &pushContantData);

                    mesh->Bind(commandBuffer);
                    mesh->OnDraw(commandBuffer);
                }

                m_VulkanRenderer->EndShadowRenderPass();
            }

            m_VulkanRenderer->UpdateViewportSize(m_ViewportSize);

            {
                FL_PROFILE_SCOPE("Swap Chain Render Pass");
                m_VulkanRenderer->BeginViewportRenderPass();

                VulkanRenderCommand::SetViewport(commandBuffer, 0.0f, 0.0f, m_ViewportSize.x, m_ViewportSize.y);
                VulkanRenderCommand::SetScissor(commandBuffer, { 0, 0 }, VkExtent2D{ (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y });

                // Update Uniforms
                m_ActiveCamera.OnResize(m_ViewportSize.x / m_ViewportSize.y);

                CameraUniformBufferObject uniformBufferObject{};
                uniformBufferObject.ViewProjectionMatrix = m_ActiveCamera.GetViewProjectionMatrix();
                uniformBufferObject.LightViewProjectionMatrix = lightViewProjectionMatrix;

                m_UniformBuffers[m_VulkanRenderer->GetCurrentFrameIndex()]->WriteToBuffer(&uniformBufferObject, sizeof(uniformBufferObject), 0);

                m_MeshRenderer->OnDraw(commandBuffer, m_VulkanRenderer->GetCurrentFrameIndex(), m_VkDescriptorSets[m_VulkanRenderer->GetCurrentFrameIndex()], m_ActiveCamera, m_Meshes);
                m_VulkanRenderer->EndViewportRenderPass();
            }

            m_ImGuiLayer->Begin();
            OnUIRender();
            m_ImGuiLayer->End(commandBuffer, m_VulkanRenderer->GetCurrentFrameIndex(), m_VulkanRenderer->GetSwapChainExtent2D());

            bool isResized = m_VulkanRenderer->EndFrame();
            if (isResized)
                m_ImGuiLayer->InvalidateResources(m_VulkanRenderer);
        }
    }

    FlameberryEditor::~FlameberryEditor()
    {
        VulkanContext::GetCurrentDevice()->WaitIdle();
        vkDestroySampler(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), m_VkTextureSampler, nullptr);
        m_ImGuiLayer->OnDetach();
    }

    void FlameberryEditor::OnUIRender()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Viewport");

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

        uint32_t currentFrameIndex = m_VulkanRenderer->GetCurrentFrameIndex();
        InvalidateViewportImGuiDescriptorSet();

        ImGui::Image(reinterpret_cast<ImTextureID>(
            m_ViewportDescriptorSets[currentFrameIndex]),
            ImVec2{ m_ViewportSize.x, m_ViewportSize.y }
        );

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Begin("Statistics");
        FL_DISPLAY_SCOPE_DETAILS_IMGUI();
        ImGui::End();
    }

    void FlameberryEditor::InvalidateViewportImGuiDescriptorSet()
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

    std::shared_ptr<Application> Application::CreateClientApp()
    {
        return std::make_shared<FlameberryEditor>();
    }
}