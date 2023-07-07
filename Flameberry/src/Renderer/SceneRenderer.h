#pragma once

#include "Vulkan/Pipeline.h"
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/StaticMesh.h"
#include "Vulkan/DescriptorSet.h"

#include "Light.h"
#include "PerspectiveCamera.h"

#include "ECS/Scene.h"
#include "ECS/Components.h"

#define SHADOW_MAP_CASCADE_COUNT 4

namespace Flameberry {
    struct ModelMatrixPushConstantData { glm::mat4 ModelMatrix; };
    struct MousePickingPushConstantData { glm::mat4 ModelMatrix; int EntityIndex; };

    class SceneRenderer
    {
    public:
        SceneRenderer(const std::shared_ptr<DescriptorSetLayout>& globalDescriptorLayout, const std::shared_ptr<RenderPass>& renderPass, const std::vector<VkImageView>& shadowMapImageViews, VkSampler shadowMapSampler);
        ~SceneRenderer();

        void OnDraw(
            VkDescriptorSet globalDescriptorSet,
            const PerspectiveCamera& activeCamera,
            const std::shared_ptr<Scene>& scene,
            const glm::vec2& framebufferSize,
            const std::array<glm::mat4, SHADOW_MAP_CASCADE_COUNT>& cascadeMatrices,
            const std::array<float, SHADOW_MAP_CASCADE_COUNT>& cascadeSplits,
            bool colorCascades,
            const fbentt::entity& selectedEntity = {}
        );
        void OnDrawForShadowPass(VkPipelineLayout shadowMapPipelineLayout, const std::shared_ptr<Scene>& scene);
        void OnDrawForMousePickingPass(VkPipelineLayout mousePickingPipelineLayout, const std::shared_ptr<Scene>& scene);

        void SubmitMesh(AssetHandle handle, const MaterialTable& materialTable, const glm::mat4& transform);
    private:
        std::shared_ptr<Pipeline> m_MeshPipeline;
        std::vector<std::shared_ptr<DescriptorSet>> m_SceneDataDescriptorSets;
        std::shared_ptr<DescriptorSetLayout> m_SceneDescriptorSetLayout;

        std::unique_ptr<Flameberry::Buffer> m_SceneUniformBuffers[VulkanSwapChain::MAX_FRAMES_IN_FLIGHT];

        std::shared_ptr<Pipeline> m_OutlinePipeline;
        std::shared_ptr<Pipeline> m_SkyboxPipeline;
    };
}
