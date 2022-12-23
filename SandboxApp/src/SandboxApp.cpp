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
    m_Texture = std::make_unique<Flameberry::VulkanTexture>(Flameberry::VulkanRenderer::GetDevice(), FL_PROJECT_DIR"SandboxApp/assets/textures/StoneIdol.jpg");

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

    // Create Pipeline Layout
    VkPushConstantRange vk_push_constant_range{};
    vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    vk_push_constant_range.size = sizeof(Flameberry::ModelMatrixPushConstantData);
    vk_push_constant_range.offset = 0;

    VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
    vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vk_pipeline_layout_create_info.setLayoutCount = 1;
    auto layout = m_VulkanDescriptorLayout->GetLayout();
    vk_pipeline_layout_create_info.pSetLayouts = &layout;
    vk_pipeline_layout_create_info.pushConstantRangeCount = 1;
    vk_pipeline_layout_create_info.pPushConstantRanges = &vk_push_constant_range;

    FL_ASSERT(vkCreatePipelineLayout(Flameberry::VulkanRenderer::GetDevice(), &vk_pipeline_layout_create_info, nullptr, &m_VkPipelineLayout) == VK_SUCCESS, "Failed to create Vulkan pipeline layout!");

    // Create Pipeline
    Flameberry::VulkanPipelineSpecification pipelineSpec{};
    pipelineSpec.vertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/triangleVert.spv";
    pipelineSpec.fragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/triangleFrag.spv";
    pipelineSpec.renderPass = Flameberry::VulkanRenderer::GetRenderPass();
    pipelineSpec.subPass = 0;

    pipelineSpec.pipelineLayout = m_VkPipelineLayout;

    VkVertexInputBindingDescription vk_vertex_input_binding_description = Flameberry::VulkanVertex::GetBindingDescription();
    auto vk_attribute_descriptions = Flameberry::VulkanVertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info{};
    vk_pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vk_vertex_input_binding_description;
    vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vk_attribute_descriptions.size());
    vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vk_attribute_descriptions.data();

    pipelineSpec.pipelineVertexInputStateCreateInfo = vk_pipeline_vertex_input_state_create_info;

    Flameberry::VulkanPipeline::FillWithDefaultPipelineSpecification(pipelineSpec);

    m_VulkanPipeline = std::make_unique<Flameberry::VulkanPipeline>(Flameberry::VulkanRenderer::GetDevice(), pipelineSpec);

    // Vertex Buffer and Index Buffer
    auto [vk_vertices, indices] = Flameberry::VulkanRenderCommand::LoadModel(FL_PROJECT_DIR"SandboxApp/assets/models/sphere.obj");
    memcpy(m_Indices, indices.data(), sizeof(uint32_t) * indices.size());

    {
        // Creating Vertex Buffer
        VkDeviceSize bufferSize = sizeof(Flameberry::VulkanVertex) * vk_vertices.size();
        Flameberry::VulkanBuffer stagingBuffer(Flameberry::VulkanRenderer::GetDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        stagingBuffer.MapMemory(bufferSize);
        stagingBuffer.WriteToBuffer(vk_vertices.data(), bufferSize, 0);
        stagingBuffer.UnmapMemory();

        m_VertexBuffer = std::make_unique<Flameberry::VulkanBuffer>(Flameberry::VulkanRenderer::GetDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        Flameberry::VulkanRenderer::CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer->GetBuffer(), bufferSize);
    }

    {
        // Creating Index Buffer
        VkDeviceSize bufferSize = sizeof(m_Indices);
        Flameberry::VulkanBuffer stagingBuffer(Flameberry::VulkanRenderer::GetDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        stagingBuffer.MapMemory(bufferSize);
        stagingBuffer.WriteToBuffer(indices.data(), bufferSize, 0);
        stagingBuffer.UnmapMemory();

        m_IndexBuffer = std::make_unique<Flameberry::VulkanBuffer>(Flameberry::VulkanRenderer::GetDevice(), bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        Flameberry::VulkanRenderer::CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer->GetBuffer(), bufferSize);
    }
}

void SandboxApp::OnUpdate(float delta)
{
    m_ActiveCamera.OnUpdate(delta);
    if (VkCommandBuffer commandBuffer = Flameberry::VulkanRenderer::BeginFrame())
    {
        Flameberry::VulkanRenderer::BeginRenderPass();
        m_VulkanPipeline->Bind(commandBuffer);

        // Update Uniforms
        m_ActiveCamera.OnResize((float)Flameberry::VulkanRenderer::GetSwapChainExtent2D().width / (float)Flameberry::VulkanRenderer::GetSwapChainExtent2D().height);
        Flameberry::CameraUniformBufferObject uniformBufferObject{};
        uniformBufferObject.ViewProjectionMatrix = m_ActiveCamera.GetViewProjectionMatrix();
        m_UniformBuffers[Flameberry::VulkanRenderer::GetCurrentFrameIndex()]->WriteToBuffer(&uniformBufferObject, sizeof(uniformBufferObject), 0);

        VkBuffer vk_vertex_buffers[] = { m_VertexBuffer->GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vk_vertex_buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Flameberry::ModelMatrixPushConstantData pushConstantData;
        pushConstantData.ModelMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // glm::mat4 modelMatrix(1.0f);

        vkCmdPushConstants(commandBuffer, m_VkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Flameberry::ModelMatrixPushConstantData), &pushConstantData);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkPipelineLayout, 0, 1, &m_VkDescriptorSets[Flameberry::VulkanRenderer::GetCurrentFrameIndex()], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, sizeof(m_Indices) / sizeof(uint32_t), 1, 0, 0, 0);

        Flameberry::VulkanRenderer::EndRenderPass();
        Flameberry::VulkanRenderer::EndFrame();
    }
}

SandboxApp::~SandboxApp()
{
    m_Texture->~VulkanTexture();

    m_VertexBuffer->DestroyBuffer();
    m_IndexBuffer->DestroyBuffer();

    for (auto& uniformBuffer : m_UniformBuffers)
        uniformBuffer->DestroyBuffer();

    m_VulkanDescriptorLayout->~VulkanDescriptorLayout();
    m_VulkanDescriptorLayout.release();

    m_VulkanDescriptorPool->~VulkanDescriptorPool();
    m_VulkanDescriptorPool.release();

    vkDestroyPipelineLayout(Flameberry::VulkanRenderer::GetDevice(), m_VkPipelineLayout, nullptr);
    m_VulkanPipeline->~VulkanPipeline();
    m_VulkanPipeline.release();
    Flameberry::VulkanRenderer::CleanUp();
}

void SandboxApp::OnUIRender()
{
}

std::shared_ptr<Flameberry::Application> Flameberry::Application::CreateClientApp()
{
    return std::make_shared<SandboxApp>();
}
