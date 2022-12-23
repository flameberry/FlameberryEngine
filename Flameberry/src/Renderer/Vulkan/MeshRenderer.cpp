#include "MeshRenderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Core/Core.h"
#include "VulkanRenderer.h"
#include "VulkanMesh.h"

namespace Flameberry {
    MeshRenderer::MeshRenderer(VkDevice& device, VkDescriptorSetLayout descriptorLayout)
        : m_VkDevice(device)
    {
        VkPushConstantRange vk_push_constant_range{};
        vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        vk_push_constant_range.size = sizeof(ModelMatrixPushConstantData);
        vk_push_constant_range.offset = 0;

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = 1;
        vk_pipeline_layout_create_info.pSetLayouts = &descriptorLayout;
        vk_pipeline_layout_create_info.pushConstantRangeCount = 1;
        vk_pipeline_layout_create_info.pPushConstantRanges = &vk_push_constant_range;

        FL_ASSERT(vkCreatePipelineLayout(m_VkDevice, &vk_pipeline_layout_create_info, nullptr, &m_VkPipelineLayout) == VK_SUCCESS, "Failed to create Vulkan pipeline layout!");

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

        m_MeshPipeline = std::make_unique<Flameberry::VulkanPipeline>(m_VkDevice, pipelineSpec);
    }

    void MeshRenderer::OnDraw(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSet, std::vector<std::shared_ptr<VulkanMesh>>& meshes)
    {
        m_MeshPipeline->Bind(commandBuffer);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkPipelineLayout, 0, 1, descriptorSet, 0, nullptr);

        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        for (auto& mesh : meshes)
        {
            ModelMatrixPushConstantData pushConstantData;
            pushConstantData.ModelMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            // pushConstantData.ModelMatrix = glm::mat4(1.0f);

            vkCmdPushConstants(commandBuffer, m_VkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrixPushConstantData), &pushConstantData);

            mesh->Bind(commandBuffer);
            mesh->OnDraw(commandBuffer);
        }
    }

    MeshRenderer::~MeshRenderer()
    {
        vkDestroyPipelineLayout(m_VkDevice, m_VkPipelineLayout, nullptr);
    }
}