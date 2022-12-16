#pragma once

#include <string>

#include <vulkan/vulkan.h>

namespace Flameberry {
    struct VulkanPipelineSpecification
    {
        std::string vertexShaderFilePath, fragmentShaderFilePath;
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo;
        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo;
        VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo;
        VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
        VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
        VkRenderPass renderPass;
        uint32_t subPass;
        VkPipelineLayout pipelineLayout;

        VulkanPipelineSpecification() = default;
    };

    class VulkanPipeline
    {
    public:
        VulkanPipeline(VkDevice& device, const VulkanPipelineSpecification& pipelineSpec);
        ~VulkanPipeline();

        void Bind(VkCommandBuffer commandBuffer);
        static void FillWithDefaultPipelineSpecification(VulkanPipelineSpecification& pipelineSpec);
    private:
        void CreatePipeline(const VulkanPipelineSpecification& pipelineSpec);
    private:
        VkDevice& m_VkDevice;
        VkPipeline m_VkGraphicsPipeline;
        
        static VkViewport m_Viewport;
        static VkRect2D m_VkScissor;
    };
}
