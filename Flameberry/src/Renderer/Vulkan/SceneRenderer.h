#pragma once

#include "VulkanPipeline.h"
#include "VulkanSwapChain.h"
#include "StaticMesh.h"
#include "VulkanDescriptor.h"

#include "Renderer/Light.h"
#include "Renderer/PerspectiveCamera.h"

#include "ECS/Scene.h"

namespace Flameberry {
    class SceneRenderer
    {
    public:
        SceneRenderer(const std::shared_ptr<VulkanDescriptorPool>& globalDescriptorPool, VkDescriptorSetLayout globalDescriptorLayout, VkRenderPass renderPass);
        ~SceneRenderer();

        void OnDraw(VkCommandBuffer commandBuffer, uint32_t currentFrameIndex, VkDescriptorSet globalDescriptorSet, const PerspectiveCamera& activeCamera, const std::shared_ptr<Scene>& scene);
        void OnDrawForShadowPass(VkCommandBuffer commandBuffer, VkPipelineLayout shadowMapPipelineLayout, const PerspectiveCamera& activeCamera, const std::shared_ptr<Scene>& scene);
    private:
        std::unique_ptr<VulkanPipeline> m_MeshPipeline;
        VkPipelineLayout m_VkPipelineLayout;

        std::vector<VkDescriptorSet> m_SceneDataDescriptorSets;
        std::unique_ptr<VulkanDescriptorLayout> m_SceneDescriptorLayout;
        std::unique_ptr<VulkanDescriptorWriter> m_SceneDescriptorWriter;
        std::unique_ptr<Flameberry::VulkanBuffer> m_SceneUniformBuffers[VulkanSwapChain::MAX_FRAMES_IN_FLIGHT];

        const std::shared_ptr<VulkanDescriptorPool>& m_GlobalDescriptorPool;
    };
}
