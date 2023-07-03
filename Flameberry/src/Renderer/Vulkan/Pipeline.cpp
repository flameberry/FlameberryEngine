#include "Pipeline.h"

#include "RenderCommand.h"
#include "VulkanContext.h"
#include "Renderer/Renderer.h"

#include "VulkanDebug.h"

namespace Flameberry {
    Pipeline::Pipeline(const PipelineSpecification& pipelineSpec)
        : m_PipelineSpec(pipelineSpec)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        std::vector<char> compiledVertexShader = RenderCommand::LoadCompiledShaderCode(m_PipelineSpec.VertexShaderFilePath);
        std::vector<char> compiledFragmentShader = RenderCommand::LoadCompiledShaderCode(m_PipelineSpec.FragmentShaderFilePath);

        VkShaderModule vk_vertex_shader_module = RenderCommand::CreateShaderModule(device, compiledVertexShader);
        VkShaderModule vk_fragment_shader_module = RenderCommand::CreateShaderModule(device, compiledFragmentShader);

        VkPipelineShaderStageCreateInfo vk_pipeline_vertex_shader_stage_create_info{};
        vk_pipeline_vertex_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vk_pipeline_vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vk_pipeline_vertex_shader_stage_create_info.module = vk_vertex_shader_module;
        vk_pipeline_vertex_shader_stage_create_info.pName = "main";

        VkPipelineShaderStageCreateInfo vk_pipeline_fragment_shader_stage_create_info{};
        vk_pipeline_fragment_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vk_pipeline_fragment_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        vk_pipeline_fragment_shader_stage_create_info.module = vk_fragment_shader_module;
        vk_pipeline_fragment_shader_stage_create_info.pName = "main";

        VkPipelineShaderStageCreateInfo vk_shader_stages_create_infos[2] = { vk_pipeline_vertex_shader_stage_create_info , vk_pipeline_fragment_shader_stage_create_info };

        VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
        if (m_PipelineSpec.BlendingEnable)
        {
            pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            pipelineColorBlendAttachmentState.blendEnable = VK_TRUE;
            pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
            pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        else
        {
            pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
            pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
            pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
        pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        pipelineRasterizationStateCreateInfo.depthClampEnable = m_PipelineSpec.DepthClampEnable;
        pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        pipelineRasterizationStateCreateInfo.polygonMode = m_PipelineSpec.PolygonMode;
        pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
        pipelineRasterizationStateCreateInfo.cullMode = m_PipelineSpec.CullMode;
        pipelineRasterizationStateCreateInfo.frontFace = m_PipelineSpec.FrontFace;
        pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
        pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
        pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f;
        pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
        pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        pipelineColorBlendStateCreateInfo.attachmentCount = 1;
        pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;
        pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
        pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
        pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
        pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
        pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        pipelineDepthStencilStateCreateInfo.depthTestEnable = (VkBool32)m_PipelineSpec.DepthTestEnable;
        pipelineDepthStencilStateCreateInfo.depthWriteEnable = (VkBool32)m_PipelineSpec.DepthWriteEnable;
        pipelineDepthStencilStateCreateInfo.depthCompareOp = m_PipelineSpec.DepthCompareOp;
        pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        pipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f; // Optional
        pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f; // Optional
        pipelineDepthStencilStateCreateInfo.stencilTestEnable = (VkBool32)m_PipelineSpec.StencilTestEnable;
        pipelineDepthStencilStateCreateInfo.front = m_PipelineSpec.StencilOpState;
        pipelineDepthStencilStateCreateInfo.back = m_PipelineSpec.StencilOpState;

        const auto& vertexAttributeDesc = m_PipelineSpec.VertexLayout.CreateVertexInputAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
        pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = m_PipelineSpec.VertexInputBindingDescription.stride ? 1 : 0;
        pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &m_PipelineSpec.VertexInputBindingDescription;
        pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDesc.size());
        pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDesc.data();

        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
        pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipelineInputAssemblyStateCreateInfo.topology = m_PipelineSpec.PrimitiveTopology;
        pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
        VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};

        std::vector<VkDynamicState> dynamicStates;

        if (m_PipelineSpec.DynamicStencilEnable)
            dynamicStates.emplace_back(VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE);
        if (m_PipelineSpec.DynamicStencilOp)
            dynamicStates.emplace_back(VK_DYNAMIC_STATE_STENCIL_OP);

        if (m_PipelineSpec.Viewport.width == 0 || m_PipelineSpec.Viewport.width == 0)
        {
            pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            pipelineViewportStateCreateInfo.viewportCount = 1;
            pipelineViewportStateCreateInfo.pViewports = nullptr;

            dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
        }
        else
        {
            pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            pipelineViewportStateCreateInfo.viewportCount = 1;
            pipelineViewportStateCreateInfo.pViewports = &m_PipelineSpec.Viewport;
        }

        if (m_PipelineSpec.Scissor.extent.width == 0 || m_PipelineSpec.Scissor.extent.height == 0)
        {
            pipelineViewportStateCreateInfo.scissorCount = 1;
            pipelineViewportStateCreateInfo.pScissors = nullptr;

            dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);
        }
        else
        {
            pipelineViewportStateCreateInfo.scissorCount = 1;
            pipelineViewportStateCreateInfo.pScissors = &m_PipelineSpec.Scissor;
        }
        pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipelineDynamicStateCreateInfo.dynamicStateCount = (uint32_t)dynamicStates.size();
        pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
        pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipelineMultisampleStateCreateInfo.minSampleShading = 1.0f;
        pipelineMultisampleStateCreateInfo.pSampleMask = nullptr;
        pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

        if (m_PipelineSpec.Samples > 1)
        {
            pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_TRUE;
            pipelineMultisampleStateCreateInfo.rasterizationSamples = (VkSampleCountFlagBits)m_PipelineSpec.Samples;
        }
        else
        {
            pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
            pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        }

        // Creating Actual Vulkan Graphics Pipeline
        VkGraphicsPipelineCreateInfo vk_graphics_pipeline_create_info{};
        vk_graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        vk_graphics_pipeline_create_info.stageCount = 2;
        vk_graphics_pipeline_create_info.pStages = vk_shader_stages_create_infos;
        vk_graphics_pipeline_create_info.pVertexInputState = &pipelineVertexInputStateCreateInfo;
        vk_graphics_pipeline_create_info.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
        vk_graphics_pipeline_create_info.pViewportState = &pipelineViewportStateCreateInfo;
        vk_graphics_pipeline_create_info.pRasterizationState = &pipelineRasterizationStateCreateInfo;
        vk_graphics_pipeline_create_info.pMultisampleState = &pipelineMultisampleStateCreateInfo;
        vk_graphics_pipeline_create_info.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
        vk_graphics_pipeline_create_info.pColorBlendState = &pipelineColorBlendStateCreateInfo;
        vk_graphics_pipeline_create_info.pDynamicState = &pipelineDynamicStateCreateInfo;
        vk_graphics_pipeline_create_info.layout = m_PipelineSpec.PipelineLayout;
        vk_graphics_pipeline_create_info.renderPass = m_PipelineSpec.RenderPass->GetRenderPass();
        vk_graphics_pipeline_create_info.subpass = m_PipelineSpec.SubPass;
        vk_graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        vk_graphics_pipeline_create_info.basePipelineIndex = -1;

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &vk_graphics_pipeline_create_info, nullptr, &m_VkGraphicsPipeline));

        // Destroying Shader Modules
        vkDestroyShaderModule(device, vk_vertex_shader_module, nullptr);
        vkDestroyShaderModule(device, vk_fragment_shader_module, nullptr);
    }

    Pipeline::~Pipeline()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyPipeline(device, m_VkGraphicsPipeline, nullptr);
    }

    void Pipeline::Bind()
    {
        const auto& pipeline = m_VkGraphicsPipeline;
        Renderer::Submit([pipeline](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
            {
                vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            }
        );
    }
}
