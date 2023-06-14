#pragma once

#include "Vulkan/Pipeline.h"
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/StaticMesh.h"
#include "Vulkan/DescriptorSet.h"

#include "Light.h"
#include "PerspectiveCamera.h"

#include "ECS/Scene.h"

namespace Flameberry {
    struct ModelMatrixPushConstantData { glm::mat4 ModelMatrix; };
    struct MousePickingPushConstantData { glm::mat4 ModelMatrix; int EntityID; };

    class SceneRenderer
    {
    public:
        SceneRenderer(VkDescriptorSetLayout globalDescriptorLayout, const std::shared_ptr<RenderPass>& renderPass, const std::vector<VkImageView>& shadowMapImageViews, VkSampler shadowMapSampler);
        ~SceneRenderer();

        void OnDraw(VkDescriptorSet globalDescriptorSet, const PerspectiveCamera& activeCamera, const std::shared_ptr<Scene>& scene, const glm::vec2& framebufferSize, const fbentt::entity_handle& selectedEntity = {});
        void OnDrawForShadowPass(VkPipelineLayout shadowMapPipelineLayout, const std::shared_ptr<Scene>& scene);
        void OnDrawForMousePickingPass(VkPipelineLayout mousePickingPipelineLayout, const std::shared_ptr<Scene>& scene);
    private:
        std::shared_ptr<Pipeline> m_MeshPipeline;
        VkPipelineLayout m_VkPipelineLayout;

        std::vector<std::shared_ptr<DescriptorSet>> m_SceneDataDescriptorSets;
        std::shared_ptr<DescriptorSetLayout> m_SceneDescriptorSetLayout;

        std::unique_ptr<Flameberry::Buffer> m_SceneUniformBuffers[VulkanSwapChain::MAX_FRAMES_IN_FLIGHT];

        std::shared_ptr<Pipeline> m_OutlinePipeline;
        VkPipelineLayout m_OutlinePipelineLayout;
    };
}
