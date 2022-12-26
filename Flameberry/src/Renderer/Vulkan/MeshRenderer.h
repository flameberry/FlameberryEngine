#pragma once

#include "VulkanPipeline.h"
#include "VulkanMesh.h"

namespace Flameberry {
    class MeshRenderer
    {
    public:
        MeshRenderer(VkDescriptorSetLayout descriptorLayout, VkRenderPass renderPass);
        ~MeshRenderer();

        void OnDraw(VkCommandBuffer commandBuffer, VkDescriptorSet* descriptorSet, std::vector<std::shared_ptr<VulkanMesh>>& meshes);
    private:
        std::unique_ptr<VulkanPipeline> m_MeshPipeline;
        VkPipelineLayout m_VkPipelineLayout;
    };
}
