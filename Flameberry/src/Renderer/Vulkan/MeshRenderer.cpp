#include "MeshRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Core/Core.h"
#include "VulkanRenderer.h"
#include "VulkanMesh.h"
#include "Renderer/Material.h"

namespace Flameberry {
    struct SceneData {
        glm::vec3 cameraPosition;
        DirectionalLight directionalLight;
    };

    struct MeshData {
        alignas(16) glm::mat4 ModelMatrix;
        Material MeshMaterial;
    };

    MeshRenderer::MeshRenderer(VulkanDescriptorPool& globalDescriptorPool, VkDescriptorSetLayout globalDescriptorLayout, VkRenderPass renderPass)
        : m_GlobalDescriptorPool(globalDescriptorPool)
    {
        FL_LOG(alignof(Material));
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        m_SceneUniformBuffer = std::make_unique<Flameberry::VulkanBuffer>(sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_SceneUniformBuffer->MapMemory(sizeof(SceneData));

        VkDescriptorSetLayoutBinding sceneDataBinding{};
        sceneDataBinding.binding = 0;
        sceneDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sceneDataBinding.descriptorCount = 1;
        sceneDataBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        sceneDataBinding.pImmutableSamplers = nullptr;

        std::vector<VkDescriptorSetLayoutBinding> bindings = { sceneDataBinding };

        m_SceneDescriptorLayout = std::make_unique<Flameberry::VulkanDescriptorLayout>(bindings);
        m_SceneDescriptorWriter = std::make_unique<Flameberry::VulkanDescriptorWriter>(*m_SceneDescriptorLayout);

        VkDescriptorBufferInfo vk_descriptor_lighting_buffer_info{};
        vk_descriptor_lighting_buffer_info.buffer = m_SceneUniformBuffer->GetBuffer();
        vk_descriptor_lighting_buffer_info.offset = 0;
        vk_descriptor_lighting_buffer_info.range = sizeof(SceneData);

        m_GlobalDescriptorPool.AllocateDescriptorSet(&m_SceneDataDescriptorSet, m_SceneDescriptorLayout->GetLayout());
        m_SceneDescriptorWriter->WriteBuffer(0, &vk_descriptor_lighting_buffer_info);
        m_SceneDescriptorWriter->Update(m_SceneDataDescriptorSet);

        VkPushConstantRange vk_push_constant_range{};
        vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        vk_push_constant_range.size = sizeof(MeshData);
        vk_push_constant_range.offset = 0;

        VkDescriptorSetLayout descriptorSetLayouts[] = { globalDescriptorLayout, m_SceneDescriptorLayout->GetLayout() };
        VkPushConstantRange pushConstantRanges[] = { vk_push_constant_range };

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = sizeof(descriptorSetLayouts) / sizeof(VkDescriptorSetLayout);
        vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
        vk_pipeline_layout_create_info.pushConstantRangeCount = sizeof(pushConstantRanges) / sizeof(VkPushConstantRange);
        vk_pipeline_layout_create_info.pPushConstantRanges = pushConstantRanges;

        FL_ASSERT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_VkPipelineLayout) == VK_SUCCESS, "Failed to create Vulkan pipeline layout!");

        Flameberry::VulkanPipelineSpecification pipelineSpec{};
        pipelineSpec.vertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/triangleVert.spv";
        pipelineSpec.fragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/triangleFrag.spv";
        pipelineSpec.renderPass = renderPass;
        pipelineSpec.subPass = 0;

        pipelineSpec.pipelineLayout = m_VkPipelineLayout;

        VkVertexInputBindingDescription vk_vertex_input_binding_description = Flameberry::VulkanVertex::GetBindingDescription();
        const auto& vk_attribute_descriptions = Flameberry::VulkanVertex::GetAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info{};
        vk_pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vk_vertex_input_binding_description;
        vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vk_attribute_descriptions.size());
        vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vk_attribute_descriptions.data();

        pipelineSpec.pipelineVertexInputStateCreateInfo = vk_pipeline_vertex_input_state_create_info;

        Flameberry::VulkanPipeline::FillWithDefaultPipelineSpecification(pipelineSpec);

        m_MeshPipeline = std::make_unique<Flameberry::VulkanPipeline>(pipelineSpec);
    }

    void MeshRenderer::OnDraw(VkCommandBuffer commandBuffer, VkDescriptorSet globalDescriptorSet, const PerspectiveCamera& activeCamera, std::vector<std::shared_ptr<VulkanMesh>>& meshes)
    {
        m_MeshPipeline->Bind(commandBuffer);

        SceneData sceneData;
        sceneData.cameraPosition = activeCamera.GetPosition();
        sceneData.directionalLight.Direction = glm::vec3(-1.0f);
        sceneData.directionalLight.Color = glm::vec3(1.0f);
        sceneData.directionalLight.Intensity = 1.0f;

        m_SceneUniformBuffer->WriteToBuffer(&sceneData, sizeof(SceneData), 0);

        VkDescriptorSet descriptorSets[] = { globalDescriptorSet, m_SceneDataDescriptorSet };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkPipelineLayout, 0, sizeof(descriptorSets) / sizeof(VkDescriptorSet), descriptorSets, 0, nullptr);

        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        for (auto& mesh : meshes)
        {
            MeshData pushConstantMeshData;
            pushConstantMeshData.ModelMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            // pushConstantMeshData.ModelMatrix = glm::mat4(1.0f);
            pushConstantMeshData.MeshMaterial = { glm::vec3(1, 0, 1), 0.2f, false };

            vkCmdPushConstants(commandBuffer, m_VkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshData), &pushConstantMeshData);

            mesh->Bind(commandBuffer);
            mesh->OnDraw(commandBuffer);
        }
    }

    MeshRenderer::~MeshRenderer()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyPipelineLayout(device, m_VkPipelineLayout, nullptr);
    }
}
