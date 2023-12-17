#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "VulkanVertex.h"
#include "RenderPass.h"
#include "DescriptorSet.h"
#include "Shader.h"

namespace Flameberry {

    struct PushConstantSpecification {
        VkShaderStageFlags ShaderStage;
        uint32_t Size;
    };

    struct PipelineLayoutSpecification
    {
        std::vector<PushConstantSpecification> PushConstants;
        std::vector<Ref<DescriptorSetLayout>> DescriptorSetLayouts;
    };

    struct PipelineSpecification
    {
        Ref<RenderPass> RenderPass;
        Ref<Shader> VertexShader, FragmentShader;

        PipelineLayoutSpecification PipelineLayout;
        uint32_t SubPass = 0;
        VertexInputAttributeLayout VertexLayout;
        VkVertexInputBindingDescription VertexInputBindingDescription;
        uint32_t Samples = 1;
        VkViewport Viewport = { 0, 0, 0, 0, 0.0f, 1.0f };
        VkRect2D Scissor;
        bool BlendingEnable = false, DepthTestEnable = true, DepthWriteEnable = true, DepthClampEnable = false, StencilTestEnable = false;
        VkCompareOp DepthCompareOp = VK_COMPARE_OP_LESS;
        bool DynamicStencilEnable = false, DynamicStencilOp = false;
        VkStencilOpState StencilOpState = {};
        VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
        VkPrimitiveTopology PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    };

    class Pipeline
    {
    public:
        Pipeline(const PipelineSpecification& pipelineSpec);
        ~Pipeline();

        PipelineSpecification GetSpecification() const { return m_PipelineSpec; }
        VkPipelineLayout GetLayout() const { return m_VkPipelineLayout; }

        void ReloadShaders();
        void Bind();
    private:
        void CreatePipeline();
    private:
        PipelineSpecification m_PipelineSpec;
        VkPipeline m_VkGraphicsPipeline;
        VkPipelineLayout m_VkPipelineLayout;
    };

    struct ComputePipelineSpecification {
        std::string ComputeShaderFilePath;
        PipelineLayoutSpecification PipelineLayout;
    };

    class ComputePipeline
    {
    public:
        ComputePipeline(const ComputePipelineSpecification& pipelineSpec);
        ~ComputePipeline();

        ComputePipelineSpecification GetSpecification() const { return m_PipelineSpec; }
        VkPipelineLayout GetLayout() const { return m_VkPipelineLayout; }

        void Bind();
    private:
        ComputePipelineSpecification m_PipelineSpec;
        VkPipeline m_VkComputePipeline;
        VkPipelineLayout m_VkPipelineLayout;
    };

}
