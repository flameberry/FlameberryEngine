#include "SandboxApp.h"

// Includes the Entrypoint of the main application
#include "Core/EntryPoint.h"

SandboxApp::SandboxApp()
{
    Flameberry::PerspectiveCameraInfo cameraInfo{};
    cameraInfo.aspectRatio = Flameberry::Application::Get().GetWindow().GetWidth() / Flameberry::Application::Get().GetWindow().GetHeight();
    cameraInfo.FOV = 45.0f;
    cameraInfo.zNear = 0.1f;
    cameraInfo.zFar = 1000.0f;
    cameraInfo.cameraPostion = glm::vec3(0.0f, 0.0f, 4.0f);
    cameraInfo.cameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);

    m_ActiveCamera = Flameberry::PerspectiveCamera(cameraInfo);

    m_VulkanRenderer = std::make_shared<Flameberry::VulkanRenderer>((Flameberry::VulkanWindow*)&Flameberry::Application::Get().GetWindow());
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

    std::vector<VkDescriptorSetLayoutBinding> bindings = { vk_uniform_buffer_object_layout_binding, samplerLayoutBinding };

    m_VulkanDescriptorLayout = std::make_unique<Flameberry::VulkanDescriptorLayout>(bindings);

    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Flameberry::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Flameberry::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT }
    };
    m_VulkanDescriptorPool = std::make_unique<Flameberry::VulkanDescriptorPool>(poolSizes);
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

        m_VulkanDescriptorPool->AllocateDescriptorSet(&m_VkDescriptorSets[i], m_VulkanDescriptorLayout->GetLayout());
        m_VulkanDescriptorWriter->WriteBuffer(0, &vk_descriptor_buffer_info);
        m_VulkanDescriptorWriter->WriteImage(1, &vk_image_info);
        m_VulkanDescriptorWriter->Update(m_VkDescriptorSets[i]);
    }

    m_MeshRenderer = std::make_unique<Flameberry::MeshRenderer>(m_VulkanDescriptorLayout->GetLayout(), m_VulkanRenderer->GetRenderPass());

    {
        auto [vk_vertices, indices] = Flameberry::VulkanRenderCommand::LoadModel(FL_PROJECT_DIR"SandboxApp/assets/models/sphere.obj");
        m_Meshes.emplace_back(std::make_shared<Flameberry::VulkanMesh>(vk_vertices, indices));
    }
    {
        auto [vk_vertices, indices] = Flameberry::VulkanRenderCommand::LoadModel(FL_PROJECT_DIR"SandboxApp/assets/models/sponza.obj");
        m_Meshes.emplace_back(std::make_shared<Flameberry::VulkanMesh>(vk_vertices, indices));
    }
}

void SandboxApp::OnUpdate(float delta)
{
    m_ActiveCamera.OnUpdate(delta);
    if (VkCommandBuffer commandBuffer = m_VulkanRenderer->BeginFrame())
    {
        m_VulkanRenderer->BeginRenderPass();

        // Set Viewport and Scissor
        VkViewport vk_viewport{};
        vk_viewport.x = 0.0f;
        vk_viewport.y = 0.0f;
        vk_viewport.width = (float)m_VulkanRenderer->GetSwapChainExtent2D().width;
        vk_viewport.height = (float)m_VulkanRenderer->GetSwapChainExtent2D().height;
        vk_viewport.minDepth = 0.0f;
        vk_viewport.maxDepth = 1.0f;

        VkRect2D vk_scissor_rect_2D{};
        vk_scissor_rect_2D.offset = { 0, 0 };
        vk_scissor_rect_2D.extent = m_VulkanRenderer->GetSwapChainExtent2D();

        vkCmdSetViewport(commandBuffer, 0, 1, &vk_viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &vk_scissor_rect_2D);

        // Update Uniforms
        m_ActiveCamera.OnResize((float)m_VulkanRenderer->GetSwapChainExtent2D().width / (float)m_VulkanRenderer->GetSwapChainExtent2D().height);
        Flameberry::CameraUniformBufferObject uniformBufferObject{};
        uniformBufferObject.ViewProjectionMatrix = m_ActiveCamera.GetViewProjectionMatrix();
        m_UniformBuffers[m_VulkanRenderer->GetCurrentFrameIndex()]->WriteToBuffer(&uniformBufferObject, sizeof(uniformBufferObject), 0);

        m_MeshRenderer->OnDraw(commandBuffer, &m_VkDescriptorSets[m_VulkanRenderer->GetCurrentFrameIndex()], m_Meshes);

        m_VulkanRenderer->EndRenderPass();
        m_VulkanRenderer->EndFrame();
    }
}

SandboxApp::~SandboxApp()
{
    Flameberry::VulkanContext::GetCurrentDevice()->WaitIdle();
}

void SandboxApp::OnUIRender()
{
}

std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
{
    return std::make_shared<SandboxApp>();
}
