#include "FlameberryEditor.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

FlameberryEditor::FlameberryEditor()
{
    m_VulkanRenderer = Flameberry::VulkanRenderer::Create((Flameberry::VulkanWindow*)&Flameberry::Application::Get().GetWindow());

    Flameberry::PerspectiveCameraInfo cameraInfo{};
    cameraInfo.aspectRatio = Flameberry::Application::Get().GetWindow().GetWidth() / Flameberry::Application::Get().GetWindow().GetHeight();
    cameraInfo.FOV = 45.0f;
    cameraInfo.zNear = 0.1f;
    cameraInfo.zFar = 500.0f;
    cameraInfo.cameraPostion = glm::vec3(0.0f, 0.0f, 4.0f);
    cameraInfo.cameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);

    m_ActiveCamera = Flameberry::PerspectiveCamera(cameraInfo);

    m_Texture = std::make_unique<Flameberry::VulkanTexture>(FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png");

    // Creating Uniform Buffers
    VkDeviceSize uniformBufferSize = sizeof(Flameberry::CameraUniformBufferObject);
    for (auto& uniformBuffer : m_UniformBuffers)
    {
        uniformBuffer = std::make_unique<Flameberry::VulkanBuffer>(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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

    m_VulkanDescriptorLayout = std::make_unique<Flameberry::VulkanDescriptorLayout>(bindings);
    m_VulkanDescriptorWriter = std::make_unique<Flameberry::VulkanDescriptorWriter>(*m_VulkanDescriptorLayout);

    m_VkDescriptorSets.resize(Flameberry::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < Flameberry::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo vk_descriptor_buffer_info{};
        vk_descriptor_buffer_info.buffer = m_UniformBuffers[i]->GetBuffer();
        vk_descriptor_buffer_info.offset = 0;
        vk_descriptor_buffer_info.range = sizeof(Flameberry::CameraUniformBufferObject);

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

    m_MeshRenderer = std::make_unique<Flameberry::MeshRenderer>(m_VulkanRenderer->GetGlobalDescriptorPool(), m_VulkanDescriptorLayout->GetLayout(), m_VulkanRenderer->GetRenderPass());

    {
        auto [vk_vertices, indices] = Flameberry::VulkanRenderCommand::LoadModel(FL_PROJECT_DIR"SandboxApp/assets/models/cube.obj");
        m_Meshes.emplace_back(Flameberry::VulkanMesh::Create(vk_vertices, indices));
    }
    {
        auto [vk_vertices, indices] = Flameberry::VulkanRenderCommand::LoadModel(FL_PROJECT_DIR"SandboxApp/assets/models/sponza.obj");
        m_Meshes.emplace_back(Flameberry::VulkanMesh::Create(vk_vertices, indices));
    }
    // {
    //     auto [vk_vertices, indices] = Flameberry::VulkanRenderCommand::LoadModel(FL_PROJECT_DIR"SandboxApp/assets/models/plane.obj");
    //     m_Meshes.emplace_back(Flameberry::VulkanMesh::Create(vk_vertices, indices));
    // }

    m_ImGuiLayer = std::make_unique<Flameberry::ImGuiLayer>();
    m_ImGuiLayer->OnAttach(
        m_VulkanRenderer->GetGlobalDescriptorPool()->GetDescriptorPool(),
        m_VulkanRenderer->GetSwapChainImageFormat(),
        m_VulkanRenderer->GetSwapChainImageViews(),
        m_VulkanRenderer->GetSwapChainExtent2D()
    );

    // Test
    const auto& device = Flameberry::VulkanContext::GetCurrentDevice()->GetVulkanDevice();
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.minLod = -1000;
    info.maxLod = 1000;
    info.maxAnisotropy = 1.0f;

    VK_CHECK_RESULT(vkCreateSampler(device, &info, nullptr, &m_VkTextureSampler));
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
                Flameberry::ModelMatrixPushConstantData pushContantData;
                pushContantData.ModelMatrix = glm::mat4(1.0f);
                vkCmdPushConstants(commandBuffer, m_VulkanRenderer->GetShadowMapPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Flameberry::ModelMatrixPushConstantData), &pushContantData);

                mesh->Bind(commandBuffer);
                mesh->OnDraw(commandBuffer);
            }

            m_VulkanRenderer->EndShadowRenderPass();
        }

        {
            FL_PROFILE_SCOPE("Swap Chain Render Pass");
            m_VulkanRenderer->BeginSwapChainRenderPass();

            Flameberry::VulkanRenderCommand::SetViewport(commandBuffer, 0.0f, 0.0f, (float)m_VulkanRenderer->GetSwapChainExtent2D().width, (float)m_VulkanRenderer->GetSwapChainExtent2D().height);
            Flameberry::VulkanRenderCommand::SetScissor(commandBuffer, { 0, 0 }, m_VulkanRenderer->GetSwapChainExtent2D());

            // Update Uniforms
            m_ActiveCamera.OnResize((float)m_VulkanRenderer->GetSwapChainExtent2D().width / (float)m_VulkanRenderer->GetSwapChainExtent2D().height);

            Flameberry::CameraUniformBufferObject uniformBufferObject{};
            uniformBufferObject.ViewProjectionMatrix = m_ActiveCamera.GetViewProjectionMatrix();
            uniformBufferObject.LightViewProjectionMatrix = lightViewProjectionMatrix;

            m_UniformBuffers[m_VulkanRenderer->GetCurrentFrameIndex()]->WriteToBuffer(&uniformBufferObject, sizeof(uniformBufferObject), 0);

            m_MeshRenderer->OnDraw(commandBuffer, m_VulkanRenderer->GetCurrentFrameIndex(), m_VkDescriptorSets[m_VulkanRenderer->GetCurrentFrameIndex()], m_ActiveCamera, m_Meshes);
            m_VulkanRenderer->EndSwapChainRenderPass();
        }

        m_ImGuiLayer->Begin();
        ImGui::ShowDemoWindow();
        OnUIRender();
        m_ImGuiLayer->End(commandBuffer, m_VulkanRenderer->GetCurrentFrameIndex(), m_VulkanRenderer->GetSwapChainExtent2D());

        m_VulkanRenderer->EndFrame();
    }
}

FlameberryEditor::~FlameberryEditor()
{
    Flameberry::VulkanContext::GetCurrentDevice()->WaitIdle();
    vkDestroySampler(Flameberry::VulkanContext::GetCurrentDevice()->GetVulkanDevice(), m_VkTextureSampler, nullptr);
    m_ImGuiLayer->OnDetach();
}

void FlameberryEditor::OnUIRender()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::Begin("Viewport");

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

    // auto ID = ImGui_ImplVulkan_AddTexture(m_VkTextureSampler, m_VulkanRenderer->GetSwapChainImageViews()[m_VulkanRenderer->GetCurrentFrameIndex()], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // uint64_t textureID = m_VulkanRenderer->GetFram->GetColorAttachmentId();
    // ImGui::Image(reinterpret_cast<ImTextureID>(ID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
    ImGui::End();
    ImGui::PopStyleVar();

    FL_DISPLAY_SCOPE_DETAILS_IMGUI();
}

std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
{
    return std::make_shared<FlameberryEditor>();
}
