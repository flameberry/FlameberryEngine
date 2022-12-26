#pragma once

#include <string>
#include <vector>
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
        std::vector<VkDynamicState> dynamicStatesEnabled;
        VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
        VkRenderPass renderPass;
        uint32_t subPass;
        VkPipelineLayout pipelineLayout;

        VulkanPipelineSpecification() = default;
    };

    class VulkanPipeline
    {
    public:
        VulkanPipeline(const VulkanPipelineSpecification& pipelineSpec);
        ~VulkanPipeline();

        void Bind(VkCommandBuffer commandBuffer);
        static void FillWithDefaultPipelineSpecification(VulkanPipelineSpecification& pipelineSpec);
    private:
        void CreatePipeline(const VulkanPipelineSpecification& pipelineSpec);
    private:
        VkPipeline m_VkGraphicsPipeline;
    };
}
