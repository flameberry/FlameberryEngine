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

    Flameberry::VulkanRenderer::Init(Flameberry::Application::Get().GetWindow().GetGLFWwindow());

    // Creating Texture
    m_Texture = std::make_unique<Flameberry::VulkanTexture>(Flameberry::VulkanRenderer::GetDevice(), FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png");

    // Creating Uniform Buffers
    VkDeviceSize uniformBufferSize = sizeof(Flameberry::CameraUniformBufferObject);
    for (auto& uniformBuffer : m_UniformBuffers)
    {
        uniformBuffer = std::make_unique<Flameberry::VulkanBuffer>(Flameberry::VulkanRenderer::GetDevice(), uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings = { vk_uniform_buffer_object_layout_binding, samplerLayoutBinding };

    m_VulkanDescriptorLayout = std::make_unique<Flameberry::VulkanDescriptorLayout>(Flameberry::VulkanRenderer::GetDevice(), bindings);
    m_VulkanDescriptorPool = std::make_unique<Flameberry::VulkanDescriptorPool>(Flameberry::VulkanRenderer::GetDevice());
    m_VulkanDescriptorWriter = std::make_unique<Flameberry::VulkanDescriptorWriter>(Flameberry::VulkanRenderer::GetDevice(), *m_VulkanDescriptorLayout);

    m_VkDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
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

    m_MeshRenderer = std::make_unique<Flameberry::MeshRenderer>(Flameberry::VulkanRenderer::GetDevice(), m_VulkanDescriptorLayout->GetLayout());

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
    if (VkCommandBuffer commandBuffer = Flameberry::VulkanRenderer::BeginFrame())
    {
        Flameberry::VulkanRenderer::BeginRenderPass();

        // Update Uniforms
        m_ActiveCamera.OnResize((float)Flameberry::VulkanRenderer::GetSwapChainExtent2D().width / (float)Flameberry::VulkanRenderer::GetSwapChainExtent2D().height);
        Flameberry::CameraUniformBufferObject uniformBufferObject{};
        uniformBufferObject.ViewProjectionMatrix = m_ActiveCamera.GetViewProjectionMatrix();
        m_UniformBuffers[Flameberry::VulkanRenderer::GetCurrentFrameIndex()]->WriteToBuffer(&uniformBufferObject, sizeof(uniformBufferObject), 0);

        m_MeshRenderer->OnDraw(commandBuffer, &m_VkDescriptorSets[Flameberry::VulkanRenderer::GetCurrentFrameIndex()], m_Meshes);

        Flameberry::VulkanRenderer::EndRenderPass();
        Flameberry::VulkanRenderer::EndFrame();
    }
}

SandboxApp::~SandboxApp()
{
    m_Texture->~VulkanTexture();

    for (auto& uniformBuffer : m_UniformBuffers)
        uniformBuffer->DestroyBuffer();

    m_VulkanDescriptorLayout->~VulkanDescriptorLayout();
    m_VulkanDescriptorLayout.release();

    m_VulkanDescriptorPool->~VulkanDescriptorPool();
    m_VulkanDescriptorPool.release();
    Flameberry::VulkanRenderer::CleanUp();
}

void SandboxApp::OnUIRender()
{
}

std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
{
    return std::make_shared<SandboxApp>();
}
