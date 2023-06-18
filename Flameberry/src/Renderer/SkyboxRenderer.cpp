#include "SkyboxRenderer.h"

#include <glm/gtc/type_ptr.hpp>

#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanDebug.h"
#include "Vulkan/VulkanRenderCommand.h"
#include "Vulkan/VulkanVertex.h"
#include "Renderer.h"

namespace Flameberry {
    SkyboxRenderer::SkyboxRenderer(const std::shared_ptr<RenderPass>& renderPass)
    {
        m_SkyboxTexture = VulkanTexture::TryGetOrLoadTexture("Assets/Environment/kloppenheim_02_puresky_4k.hdr");
        // m_SkyboxTexture = VulkanTexture::TryGetOrLoadTexture("Assets/Environment/little_paris_under_tower_4k.hdr");
        // m_SkyboxTexture = VulkanTexture::TryGetOrLoadTexture("Assets/Environment/sunset_fairway_4k.hdr");

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        VkDescriptorSetLayout descriptorSetLayouts[] = { VulkanTexture::GetDescriptorLayout()->GetLayout() };

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(glm::mat4);
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = sizeof(descriptorSetLayouts) / sizeof(VkDescriptorSetLayout);
        vk_pipeline_layout_create_info.pSetLayouts = descriptorSetLayouts;
        vk_pipeline_layout_create_info.pushConstantRangeCount = 1;
        vk_pipeline_layout_create_info.pPushConstantRanges = &pushConstantRange;

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_SkyboxPipelineLayout));

        Flameberry::PipelineSpecification pipelineSpec{};
        pipelineSpec.VertexShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/skybox.vert.spv";
        pipelineSpec.FragmentShaderFilePath = FL_PROJECT_DIR"Flameberry/assets/shaders/vulkan/bin/skybox.frag.spv";
        pipelineSpec.RenderPass = renderPass;
        pipelineSpec.PipelineLayout = m_SkyboxPipelineLayout;

        pipelineSpec.VertexLayout = {};

        pipelineSpec.VertexInputBindingDescription.binding = 0;
        pipelineSpec.VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        pipelineSpec.VertexInputBindingDescription.stride = 0;

        VkSampleCountFlagBits sampleCount = VulkanRenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());
        pipelineSpec.Samples = sampleCount;

        pipelineSpec.CullMode = VK_CULL_MODE_NONE;

        pipelineSpec.DepthTestEnable = true;
        pipelineSpec.DepthWriteEnable = false;
        pipelineSpec.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        m_SkyboxPipeline = Pipeline::Create(pipelineSpec);
    }

    void SkyboxRenderer::OnDraw(VkDescriptorSet globalDescriptorSet, const PerspectiveCamera& activeCamera)
    {
        m_SkyboxPipeline->Bind();

        glm::mat4 viewProjectionMatrix = activeCamera.GetProjectionMatrix() * glm::mat4(glm::mat3(activeCamera.GetViewMatrix()));

        auto pipelineLayout = m_SkyboxPipelineLayout;
        auto textureDescSet = m_SkyboxTexture->GetDescriptorSet();
        Renderer::Submit([pipelineLayout, viewProjectionMatrix, textureDescSet](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                VkDescriptorSet descSets[] = { textureDescSet };
                vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, descSets, 0, nullptr);
                vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProjectionMatrix);
                vkCmdDraw(cmdBuffer, 36, 1, 0, 0);
            }
        );
    }

    SkyboxRenderer::~SkyboxRenderer()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyPipelineLayout(device, m_SkyboxPipelineLayout, nullptr);
    }
}
