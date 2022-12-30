#pragma once

#include "VulkanPipeline.h"
#include "VulkanMesh.h"
#include "VulkanDescriptor.h"

#include "Renderer/Light.h"
#include "Renderer/PerspectiveCamera.h"

namespace Flameberry {
    class MeshRenderer
    {
    public:
        MeshRenderer(VulkanDescriptorPool& globalDescriptorPool, VkDescriptorSetLayout globalDescriptorLayout, VkRenderPass renderPass);
        ~MeshRenderer();

        void OnDraw(VkCommandBuffer commandBuffer, VkDescriptorSet globalDescriptorSet, const PerspectiveCamera& activeCamera, std::vector<std::shared_ptr<VulkanMesh>>& meshes);
    private:
        std::unique_ptr<VulkanPipeline> m_MeshPipeline;
        VkPipelineLayout m_VkPipelineLayout;

        VkDescriptorSet m_SceneDataDescriptorSet;
        std::unique_ptr<VulkanDescriptorLayout> m_SceneDescriptorLayout;
        std::unique_ptr<VulkanDescriptorWriter> m_SceneDescriptorWriter;
        std::unique_ptr<VulkanBuffer> m_SceneUniformBuffer;

        VulkanDescriptorPool& m_GlobalDescriptorPool;
    };
}
