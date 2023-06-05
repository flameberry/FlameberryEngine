#pragma once

#include "Vulkan/Pipeline.h"
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/StaticMesh.h"
#include "Vulkan/DescriptorSet.h"

#include "Light.h"
#include "PerspectiveCamera.h"

#include "ECS/Scene.h"

namespace Flameberry {
    class SceneRenderer
    {
    public:
        SceneRenderer(VkDescriptorSetLayout globalDescriptorLayout, const std::shared_ptr<RenderPass>& renderPass);
        ~SceneRenderer();

        void OnDraw(VkCommandBuffer commandBuffer, uint32_t currentFrameIndex, VkDescriptorSet globalDescriptorSet, const PerspectiveCamera& activeCamera, const std::shared_ptr<Scene>& scene);
        void OnDrawForShadowPass(VkCommandBuffer commandBuffer, VkPipelineLayout shadowMapPipelineLayout, const PerspectiveCamera& activeCamera, const std::shared_ptr<Scene>& scene);
        void OnDrawForMousePickingPass(VkCommandBuffer commandBuffer, VkPipelineLayout mousePickingPipelineLayout, const PerspectiveCamera& activeCamera, const std::shared_ptr<Scene>& scene);
    private:
        std::shared_ptr<Pipeline> m_MeshPipeline;
        VkPipelineLayout m_VkPipelineLayout;

        std::vector<std::shared_ptr<DescriptorSet>> m_SceneDataDescriptorSets;
        std::shared_ptr<DescriptorSetLayout> m_SceneDescriptorSetLayout;

        std::unique_ptr<Flameberry::VulkanBuffer> m_SceneUniformBuffers[VulkanSwapChain::MAX_FRAMES_IN_FLIGHT];
    };
}
