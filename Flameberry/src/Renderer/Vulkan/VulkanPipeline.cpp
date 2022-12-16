#include "VulkanPipeline.h"

#include "VulkanRenderCommand.h"
#include "VulkanRenderer.h"

#include "Core/Core.h"

namespace Flameberry {
    VkViewport VulkanPipeline::m_Viewport;
    VkRect2D VulkanPipeline::m_VkScissor;
    
    VulkanPipeline::VulkanPipeline(VkDevice& device, const VulkanPipelineSpecification& pipelineSpec)
        : m_VkDevice(device)
    {
        CreatePipeline(pipelineSpec);
    }

    VulkanPipeline::~VulkanPipeline()
    {
        vkDestroyPipeline(m_VkDevice, m_VkGraphicsPipeline, nullptr);
    }

    void VulkanPipeline::Bind(VkCommandBuffer commandBuffer)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkGraphicsPipeline);
    }

    void VulkanPipeline::CreatePipeline(const VulkanPipelineSpecification& pipelineSpec)
    {
        std::vector<char> compiledVertexShader = VulkanRenderCommand::LoadCompiledShaderCode(pipelineSpec.vertexShaderFilePath);
        std::vector<char> compiledFragmentShader = VulkanRenderCommand::LoadCompiledShaderCode(pipelineSpec.fragmentShaderFilePath);

        VkShaderModule vk_vertex_shader_module = VulkanRenderer::CreateShaderModule(compiledVertexShader);
        VkShaderModule vk_fragment_shader_module = VulkanRenderer::CreateShaderModule(compiledFragmentShader);

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

        // VkVertexInputBindingDescription vk_vertex_input_binding_description = VulkanVertex::GetBindingDescription();
        // auto vk_attribute_descriptions = VulkanVertex::GetAttributeDescriptions();

        // VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info{};
        // vk_pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        // vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
        // vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = &vk_vertex_input_binding_description;
        // vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vk_attribute_descriptions.size());
        // vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vk_attribute_descriptions.data();

        // Creating Actual Vulkan Graphics Pipeline
        VkGraphicsPipelineCreateInfo vk_graphics_pipeline_create_info{};
        vk_graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        vk_graphics_pipeline_create_info.stageCount = 2;
        vk_graphics_pipeline_create_info.pStages = vk_shader_stages_create_infos;
        vk_graphics_pipeline_create_info.pVertexInputState = &pipelineSpec.pipelineVertexInputStateCreateInfo;
        vk_graphics_pipeline_create_info.pInputAssemblyState = &pipelineSpec.pipelineInputAssemblyStateCreateInfo;
        vk_graphics_pipeline_create_info.pViewportState = &pipelineSpec.pipelineViewportStateCreateInfo;
        vk_graphics_pipeline_create_info.pRasterizationState = &pipelineSpec.pipelineRasterizationStateCreateInfo;
        vk_graphics_pipeline_create_info.pMultisampleState = &pipelineSpec.pipelineMultisampleStateCreateInfo;
        vk_graphics_pipeline_create_info.pDepthStencilState = &pipelineSpec.pipelineDepthStencilStateCreateInfo;
        vk_graphics_pipeline_create_info.pColorBlendState = &pipelineSpec.pipelineColorBlendStateCreateInfo;
        vk_graphics_pipeline_create_info.pDynamicState = nullptr;
        vk_graphics_pipeline_create_info.layout = pipelineSpec.pipelineLayout;
        vk_graphics_pipeline_create_info.renderPass = pipelineSpec.renderPass;
        vk_graphics_pipeline_create_info.subpass = pipelineSpec.subPass;
        vk_graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        vk_graphics_pipeline_create_info.basePipelineIndex = -1;

        FL_ASSERT(vkCreateGraphicsPipelines(m_VkDevice, VK_NULL_HANDLE, 1, &vk_graphics_pipeline_create_info, nullptr, &m_VkGraphicsPipeline) == VK_SUCCESS, "Failed to create Vulkan Graphics Pipeline!");

        // Destroying Shader Modules
        vkDestroyShaderModule(m_VkDevice, vk_vertex_shader_module, nullptr);
        vkDestroyShaderModule(m_VkDevice, vk_fragment_shader_module, nullptr);
    }

    // void VulkanPipeline::CreatePipelineLayout()
    // {
        // VkPushConstantRange vk_push_constant_range{};
        // vk_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        // vk_push_constant_range.size = sizeof(ModelMatrixPushConstantData);
        // vk_push_constant_range.offset = 0;

        // VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        // vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        // vk_pipeline_layout_create_info.setLayoutCount = 1;
        // // vk_pipeline_layout_create_info.pSetLayouts = &s_VkDescriptorSetLayout;
        // vk_pipeline_layout_create_info.pushConstantRangeCount = 1;
        // vk_pipeline_layout_create_info.pPushConstantRanges = &vk_push_constant_range;

        // FL_ASSERT(vkCreatePipelineLayout(m_VkDevice, &vk_pipeline_layout_create_info, nullptr, &m_VkPipelineLayout) == VK_SUCCESS, "Failed to create Vulkan pipeline layout!");
    // }

    void VulkanPipeline::FillWithDefaultPipelineSpecification(VulkanPipelineSpecification& pipelineSpec)
    {
        VkPipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info{};
        vk_pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        vk_pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        vk_pipeline_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

        // VkViewport vk_viewport{};
        m_Viewport.x = 0.0f;
        m_Viewport.y = 0.0f;
        m_Viewport.width = (float)VulkanRenderer::GetSwapChainExtent2D().width;
        m_Viewport.height = (float)VulkanRenderer::GetSwapChainExtent2D().height;
        m_Viewport.minDepth = 0.0f;
        m_Viewport.maxDepth = 1.0f;

        // VkRect2D vk_scissor_rect_2D{};
        m_VkScissor.offset = { 0, 0 };
        m_VkScissor.extent = VulkanRenderer::GetSwapChainExtent2D();

        VkPipelineViewportStateCreateInfo vk_pipeline_viewport_state_create_info{};
        vk_pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vk_pipeline_viewport_state_create_info.viewportCount = 1;
        vk_pipeline_viewport_state_create_info.pViewports = &m_Viewport;
        vk_pipeline_viewport_state_create_info.scissorCount = 1;
        vk_pipeline_viewport_state_create_info.pScissors = &m_VkScissor;

        VkPipelineRasterizationStateCreateInfo vk_pipeline_rasterization_state_create_info{};
        vk_pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        vk_pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE;
        vk_pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        vk_pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
        vk_pipeline_rasterization_state_create_info.lineWidth = 1.0f;
        vk_pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
        vk_pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        vk_pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
        vk_pipeline_rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
        vk_pipeline_rasterization_state_create_info.depthBiasClamp = 0.0f;
        vk_pipeline_rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo vk_pipeline_multisample_state_create_info{};
        vk_pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        vk_pipeline_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
        vk_pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        vk_pipeline_multisample_state_create_info.minSampleShading = 1.0f;
        vk_pipeline_multisample_state_create_info.pSampleMask = nullptr;
        vk_pipeline_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        vk_pipeline_multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo vk_pipeline_depth_stencil_state_create_info{};
        vk_pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        vk_pipeline_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
        vk_pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
        vk_pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
        vk_pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
        vk_pipeline_depth_stencil_state_create_info.minDepthBounds = 0.0f; // Optional
        vk_pipeline_depth_stencil_state_create_info.maxDepthBounds = 1.0f; // Optional
        vk_pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
        vk_pipeline_depth_stencil_state_create_info.front = {}; // Optional
        vk_pipeline_depth_stencil_state_create_info.back = {}; // Optional

        pipelineSpec.pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        pipelineSpec.pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
        pipelineSpec.pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineSpec.pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineSpec.pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        pipelineSpec.pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineSpec.pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineSpec.pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info{};
        vk_pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        vk_pipeline_color_blend_state_create_info.logicOpEnable = VK_FALSE;
        vk_pipeline_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
        vk_pipeline_color_blend_state_create_info.attachmentCount = 1;
        vk_pipeline_color_blend_state_create_info.pAttachments = &pipelineSpec.pipelineColorBlendAttachmentState;
        vk_pipeline_color_blend_state_create_info.blendConstants[0] = 0.0f;
        vk_pipeline_color_blend_state_create_info.blendConstants[1] = 0.0f;
        vk_pipeline_color_blend_state_create_info.blendConstants[2] = 0.0f;
        vk_pipeline_color_blend_state_create_info.blendConstants[3] = 0.0f;

        pipelineSpec.pipelineInputAssemblyStateCreateInfo = vk_pipeline_input_assembly_state_create_info;
        pipelineSpec.pipelineViewportStateCreateInfo = vk_pipeline_viewport_state_create_info;
        pipelineSpec.pipelineRasterizationStateCreateInfo = vk_pipeline_rasterization_state_create_info;
        pipelineSpec.pipelineMultisampleStateCreateInfo = vk_pipeline_multisample_state_create_info;
        pipelineSpec.pipelineDepthStencilStateCreateInfo = vk_pipeline_depth_stencil_state_create_info;
        pipelineSpec.pipelineColorBlendStateCreateInfo = vk_pipeline_color_blend_state_create_info;
    }
}
