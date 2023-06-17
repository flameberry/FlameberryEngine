#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "VulkanVertex.h"
#include "RenderPass.h"

namespace Flameberry {
    struct PipelineSpecification
    {
        VkPipelineLayout PipelineLayout;
        std::string VertexShaderFilePath, FragmentShaderFilePath;
        std::shared_ptr<RenderPass> RenderPass;
        uint32_t SubPass = 0;
        VertexInputAttributeLayout VertexLayout;
        VkVertexInputBindingDescription VertexInputBindingDescription;
        uint32_t Samples = 1;
        VkViewport Viewport = { 0, 0, 0, 0, 0.0f, 1.0f };
        VkRect2D Scissor;
        bool BlendingEnable = false, DepthTestEnable = true, StencilTestEnable = false;
        bool DynamicStencilEnable = false, DynamicStencilOp = false;
        VkStencilOpState StencilOpState = {};
        VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
        VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
        VkPrimitiveTopology PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    };

    class Pipeline
    {
    public:
        Pipeline(const PipelineSpecification& pipelineSpec);
        ~Pipeline();

        PipelineSpecification GetSpecification() const { return m_PipelineSpec; }

        void Bind();

        template<typename... Args>
        static std::shared_ptr<Pipeline> Create(Args... args) { return std::make_shared<Pipeline>(std::forward<Args>(args)...); }
    private:
        PipelineSpecification m_PipelineSpec;
        VkPipeline m_VkGraphicsPipeline;
    };
}
