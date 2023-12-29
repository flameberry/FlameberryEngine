#include "Pipeline.h"

#include "RenderCommand.h"
#include "VulkanContext.h"
#include "Renderer/Renderer.h"

#include "VulkanDebug.h"

namespace Flameberry {

    Pipeline::Pipeline(const PipelineSpecification& pipelineSpec)
        : m_Specification(pipelineSpec)
    {
        CreatePipeline();
    }

    static VkFormat ShaderDataTypeToFormat(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::None:     return VK_FORMAT_UNDEFINED;           break;
            case ShaderDataType::Float:    return VK_FORMAT_R32_SFLOAT;          break;
            case ShaderDataType::Float2:   return VK_FORMAT_R32G32_SFLOAT;       break;
            case ShaderDataType::Float3:   return VK_FORMAT_R32G32B32_SFLOAT;    break;
            case ShaderDataType::Float4:   return VK_FORMAT_R32G32B32A32_SFLOAT; break;
            case ShaderDataType::UInt:     return VK_FORMAT_R32_UINT;            break;
            case ShaderDataType::UInt2:    return VK_FORMAT_R32G32_UINT;         break;
            case ShaderDataType::UInt3:    return VK_FORMAT_R32G32B32_UINT;      break;
            case ShaderDataType::UInt4:    return VK_FORMAT_R32G32B32_UINT;      break;
            case ShaderDataType::Int:      return VK_FORMAT_R32_SINT;            break;
            case ShaderDataType::Int2:     return VK_FORMAT_R32G32_SINT;         break;
            case ShaderDataType::Int3:     return VK_FORMAT_R32G32B32_SINT;      break;
            case ShaderDataType::Int4:     return VK_FORMAT_R32G32B32A32_SINT;   break;
            case ShaderDataType::Float3x3: return VK_FORMAT_R32G32B32_SFLOAT;    break;
            case ShaderDataType::Float4x4: return VK_FORMAT_R32G32B32A32_SFLOAT; break;
            case ShaderDataType::Bool:     return VK_FORMAT_R8_UINT;             break;
            default: return VK_FORMAT_UNDEFINED; break;
        }
    }

    static VkDeviceSize SizeOfShaderDataType(ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::None:     return 0;         break;
            case ShaderDataType::Float:    return 4;         break;
            case ShaderDataType::Float2:   return 4 * 2;     break;
            case ShaderDataType::Float3:   return 4 * 3;     break;
            case ShaderDataType::Float4:   return 4 * 4;     break;
            case ShaderDataType::UInt:     return 4;         break;
            case ShaderDataType::UInt2:    return 4 * 2;     break;
            case ShaderDataType::UInt3:    return 4 * 3;     break;
            case ShaderDataType::UInt4:    return 4 * 4;     break;
            case ShaderDataType::Int:      return 4;         break;
            case ShaderDataType::Int2:     return 4 * 2;     break;
            case ShaderDataType::Int3:     return 4 * 3;     break;
            case ShaderDataType::Int4:     return 4 * 4;     break;
            case ShaderDataType::Float3x3: return 4 * 3 * 3; break;
            case ShaderDataType::Float4x4: return 4 * 4 * 4; break;
            case ShaderDataType::Bool:     return 1;         break;
            case ShaderDataType::Dummy1:   return 1;         break;
            case ShaderDataType::Dummy4:   return 4;         break;
            case ShaderDataType::Dummy8:   return 8;         break;
            case ShaderDataType::Dummy12:  return 12;        break;
            case ShaderDataType::Dummy16:  return 16;        break;
        }
        return 0;
    }

    static void PopulateVulkanPushConstantRanges(std::vector<VkPushConstantRange>& outPushConstantRanges, const Ref<Shader>& shader)
    {
        std::unordered_map<uint32_t, uint32_t> pcOffsetToIndex;
        outPushConstantRanges.reserve(shader->GetPushConstantSpecifications().size());

        for (const auto& specification : shader->GetPushConstantSpecifications())
        {
            if (auto it = pcOffsetToIndex.find(specification.Offset); it != pcOffsetToIndex.end())
            {
                FBY_ASSERT(!(specification.VulkanShaderStage & outPushConstantRanges[it->second].stageFlags), "Push constant blocks within the same shader stage must not have the same offset!");
                // TODO: Maybe make this be just a warning, so that the flexibility still remains
                FBY_ASSERT(specification.Size == outPushConstantRanges[it->second].size, "Push constant blocks between different shader stages having the same offset must have the same size! (The assumption here is that both represent the same push constant block)");
                outPushConstantRanges[it->second].stageFlags |= specification.VulkanShaderStage;
            }
            else
            {
                outPushConstantRanges.emplace_back(
                    VkPushConstantRange{
                    .offset = specification.Offset,
                    .size = specification.Size,
                    .stageFlags = (VkShaderStageFlags)specification.VulkanShaderStage
                    }
                );
                pcOffsetToIndex[specification.Offset] = (uint32_t)outPushConstantRanges.size() - 1;
            }
        }
    }

    static void PopulateVulkanDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& outVulkanDescriptorSetLayouts, std::vector<Ref<DescriptorSetLayout>>& outDescriptorSetLayouts, const Ref<Shader>& shader)
    {
        // Creating the Descriptor Set Layouts required for creating the Pipeline based on the Shader Reflection Data
        const auto& reflectionDescriptorSets = shader->GetDescriptorSetSpecifications();
        const auto& descriptorBindings = shader->GetDescriptorBindingSpecifications();

        std::vector<VkDescriptorSetLayoutBinding> vulkanDescSetBindings;
        uint32_t index = 0;

        for (const auto& reflectionDescSet : reflectionDescriptorSets)
        {
            vulkanDescSetBindings.reserve(reflectionDescSet.BindingCount);
            for (uint32_t i = 0; i < reflectionDescSet.BindingCount; i++)
            {
                vulkanDescSetBindings.emplace_back(VkDescriptorSetLayoutBinding{
                    .binding = descriptorBindings[index].Binding,
                    .descriptorCount = descriptorBindings[index].Count,
                    .descriptorType = descriptorBindings[index].Type,
                    .stageFlags = descriptorBindings[index].VulkanShaderStage,
                    .pImmutableSamplers = nullptr
                    }
                );
                index++;
            }

            // Create or Get the Cached descriptor set layout
            if (vulkanDescSetBindings.size())
            {
                DescriptorSetLayoutSpecification layoutSpecification{ vulkanDescSetBindings };
                auto& layout = outDescriptorSetLayouts.emplace_back(DescriptorSetLayout::CreateOrGetCached(layoutSpecification));
                outVulkanDescriptorSetLayouts.push_back(layout->GetLayout());
                vulkanDescSetBindings.clear();
            }
        }
    }

    void Pipeline::CreatePipeline()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        // Creating Pipeline Layout
        std::vector<VkPushConstantRange> vulkanPushConstantRanges;
        PopulateVulkanPushConstantRanges(vulkanPushConstantRanges, m_Specification.Shader);

        std::vector<VkDescriptorSetLayout> vulkanDescriptorSetLayouts;
        PopulateVulkanDescriptorSetLayouts(vulkanDescriptorSetLayouts, m_DescriptorSetLayouts, m_Specification.Shader);

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = (uint32_t)vulkanDescriptorSetLayouts.size();
        vk_pipeline_layout_create_info.pSetLayouts = vulkanDescriptorSetLayouts.data();
        vk_pipeline_layout_create_info.pushConstantRangeCount = (uint32_t)vulkanPushConstantRanges.size();
        vk_pipeline_layout_create_info.pPushConstantRanges = vulkanPushConstantRanges.data();

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_PipelineLayout));

        // Creating Pipeline
        VkShaderModule vertexModule, fragmentModule;
        m_Specification.Shader->GetVertexAndFragmentShaderModules(&vertexModule, &fragmentModule);

        VkPipelineShaderStageCreateInfo vk_pipeline_vertex_shader_stage_create_info{};
        vk_pipeline_vertex_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vk_pipeline_vertex_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vk_pipeline_vertex_shader_stage_create_info.module = vertexModule;
        vk_pipeline_vertex_shader_stage_create_info.pName = "main";

        VkPipelineShaderStageCreateInfo vk_pipeline_fragment_shader_stage_create_info{};
        vk_pipeline_fragment_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vk_pipeline_fragment_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        vk_pipeline_fragment_shader_stage_create_info.module = fragmentModule;
        vk_pipeline_fragment_shader_stage_create_info.pName = "main";

        VkPipelineShaderStageCreateInfo vk_shader_stages_create_infos[2] = { vk_pipeline_vertex_shader_stage_create_info , vk_pipeline_fragment_shader_stage_create_info };

        VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
        if (m_Specification.BlendingEnable)
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
        pipelineRasterizationStateCreateInfo.depthClampEnable = m_Specification.DepthClampEnable;
        pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        pipelineRasterizationStateCreateInfo.polygonMode = m_Specification.PolygonMode;
        pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
        pipelineRasterizationStateCreateInfo.cullMode = m_Specification.CullMode;
        pipelineRasterizationStateCreateInfo.frontFace = m_Specification.FrontFace;
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
        pipelineDepthStencilStateCreateInfo.depthTestEnable = (VkBool32)m_Specification.DepthTestEnable;
        pipelineDepthStencilStateCreateInfo.depthWriteEnable = (VkBool32)m_Specification.DepthWriteEnable;
        pipelineDepthStencilStateCreateInfo.depthCompareOp = m_Specification.DepthCompareOp;
        pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        pipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f; // Optional
        pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f; // Optional
        pipelineDepthStencilStateCreateInfo.stencilTestEnable = (VkBool32)m_Specification.StencilTestEnable;
        pipelineDepthStencilStateCreateInfo.front = m_Specification.StencilOpState;
        pipelineDepthStencilStateCreateInfo.back = m_Specification.StencilOpState;

        // Create the vertex attribute description structs based on the m_PipelineSpec.VertexLayout
        VkVertexInputAttributeDescription vertexAttributeDescriptions[m_Specification.VertexLayout.size()];
        uint32_t offset = 0, idx = 0;
        for (uint32_t loc = 0; loc < m_Specification.VertexLayout.size(); loc++)
        {
            const auto dataType = m_Specification.VertexLayout[loc];
            if (dataType < ShaderDataType::Dummy1)
            {
                vertexAttributeDescriptions[idx].binding = 0;
                vertexAttributeDescriptions[idx].location = loc;
                vertexAttributeDescriptions[idx].format = ShaderDataTypeToFormat(dataType);
                vertexAttributeDescriptions[idx].offset = offset;
                idx++;
            }
            offset += SizeOfShaderDataType(dataType);
        }

        // Fill the Vertex Binding Description purely based upon the vertex layout provided
        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = offset; // The final offset will turn out to be the total stride
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
        pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexInputBindingDescription.stride ? 1 : 0;
        pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
        pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = idx; // Index will represent Size after the last increment
        pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions;

        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
        pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipelineInputAssemblyStateCreateInfo.topology = m_Specification.PrimitiveTopology;
        pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
        VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};

        std::vector<VkDynamicState> dynamicStates;

        if (m_Specification.DynamicStencilEnable)
            dynamicStates.emplace_back(VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE);
        if (m_Specification.DynamicStencilOp)
            dynamicStates.emplace_back(VK_DYNAMIC_STATE_STENCIL_OP);

        if (m_Specification.Viewport.width == 0 || m_Specification.Viewport.width == 0)
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
            pipelineViewportStateCreateInfo.pViewports = &m_Specification.Viewport;
        }

        if (m_Specification.Scissor.extent.width == 0 || m_Specification.Scissor.extent.height == 0)
        {
            pipelineViewportStateCreateInfo.scissorCount = 1;
            pipelineViewportStateCreateInfo.pScissors = nullptr;

            dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);
        }
        else
        {
            pipelineViewportStateCreateInfo.scissorCount = 1;
            pipelineViewportStateCreateInfo.pScissors = &m_Specification.Scissor;
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

        if (m_Specification.Samples > 1)
        {
            pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_TRUE;
            pipelineMultisampleStateCreateInfo.rasterizationSamples = (VkSampleCountFlagBits)m_Specification.Samples;
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
        vk_graphics_pipeline_create_info.layout = m_PipelineLayout;
        vk_graphics_pipeline_create_info.renderPass = m_Specification.RenderPass->GetRenderPass();
        vk_graphics_pipeline_create_info.subpass = m_Specification.SubPass;
        vk_graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        vk_graphics_pipeline_create_info.basePipelineIndex = -1;

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &vk_graphics_pipeline_create_info, nullptr, &m_GraphicsPipeline));
    }

    void Pipeline::ReloadShaders()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyPipeline(device, m_GraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);

        CreatePipeline();
    }

    Pipeline::~Pipeline()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyPipeline(device, m_GraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    }

    ComputePipeline::ComputePipeline(const ComputePipelineSpecification& pipelineSpec)
        : m_Specification(pipelineSpec)
    {
        const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        std::vector<VkPushConstantRange> vulkanPushConstantRanges;
        PopulateVulkanPushConstantRanges(vulkanPushConstantRanges, m_Specification.Shader);

        std::vector<VkDescriptorSetLayout> vulkanDescriptorSetLayouts;
        PopulateVulkanDescriptorSetLayouts(vulkanDescriptorSetLayouts, m_DescriptorSetLayouts, m_Specification.Shader);

        VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info{};
        vk_pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vk_pipeline_layout_create_info.setLayoutCount = (uint32_t)vulkanDescriptorSetLayouts.size();
        vk_pipeline_layout_create_info.pSetLayouts = vulkanDescriptorSetLayouts.data();
        vk_pipeline_layout_create_info.pushConstantRangeCount = (uint32_t)vulkanPushConstantRanges.size();
        vk_pipeline_layout_create_info.pPushConstantRanges = vulkanPushConstantRanges.data();

        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &vk_pipeline_layout_create_info, nullptr, &m_PipelineLayout));

        VkShaderModule computeShaderModule = m_Specification.Shader->GetVulkanShaderModule();

        VkPipelineShaderStageCreateInfo pipelineComputeShaderStageCreateInfo{};
        pipelineComputeShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineComputeShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        pipelineComputeShaderStageCreateInfo.module = computeShaderModule;
        pipelineComputeShaderStageCreateInfo.pName = "main";

        VkComputePipelineCreateInfo computePipelineCreateInfo{};
        computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineCreateInfo.layout = m_PipelineLayout;
        computePipelineCreateInfo.stage = pipelineComputeShaderStageCreateInfo;

        VK_CHECK_RESULT(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_ComputePipeline));
    }

    ComputePipeline::~ComputePipeline()
    {
        const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyPipeline(device, m_ComputePipeline, nullptr);
        vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
    }

}