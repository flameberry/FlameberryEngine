#pragma once

#include "Vulkan/Pipeline.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/StaticMesh.h"
#include "Vulkan/DescriptorSet.h"

#include "Light.h"
#include "PerspectiveCamera.h"

#include "ECS/Scene.h"
#include "ECS/Components.h"

namespace Flameberry {
    struct ModelMatrixPushConstantData { glm::mat4 ModelMatrix; };
    struct MousePickingPushConstantData { glm::mat4 ModelMatrix; int EntityIndex; };

    struct SceneRendererSettings {
        bool EnableShadows = true, ShowCascades = false, SoftShadows = true;
        float CascadeLambdaSplit = 0.91f;
        static const uint32_t CascadeCount = 4, CascadeSize = 2048;
    };

    struct Cascade {
        glm::mat4 ViewProjectionMatrix;
        float DepthSplit;
    };

    class SceneRenderer
    {
    public:
        SceneRenderer(const glm::vec2& viewportSize);
        ~SceneRenderer();

        void RenderScene(const glm::vec2& viewportSize, const std::shared_ptr<Scene>& scene, const std::shared_ptr<PerspectiveCamera>& camera, fbentt::entity selectedEntity, bool renderGrid = true);

        VkImageView GetGeometryPassOutputImageView(uint32_t index) const { return m_GeometryPass->GetSpecification().TargetFramebuffers[index]->GetColorResolveAttachment(0)->GetImageView(); }
        VkImageView GetCompositePassOutputImageView(uint32_t index) const { return m_CompositePass->GetSpecification().TargetFramebuffers[index]->GetColorAttachment(0)->GetImageView(); }
        VkDescriptorSet GetCameraBufferDescriptorSet(uint32_t index) const { return m_CameraBufferDescriptorSets[index]->GetDescriptorSet(); }

        SceneRendererSettings& GetRendererSettingsRef() { return m_RendererSettings; }
        void RenderSceneForMousePicking(VkPipelineLayout mousePickingPipelineLayout, const std::shared_ptr<Scene>& scene);
    private:
        void Init();

        void CalculateShadowMapCascades(const std::shared_ptr<PerspectiveCamera>& camera, const glm::vec3& lightDirection);
        void SubmitMesh(AssetHandle handle, const MaterialTable& materialTable, const glm::mat4& transform);
    private:
        glm::vec2 m_ViewportSize;

        // Geometry
        std::shared_ptr<RenderPass> m_GeometryPass;
        std::shared_ptr<DescriptorSetLayout> m_CameraBufferDescSetLayout;
        std::vector<std::shared_ptr<DescriptorSet>> m_CameraBufferDescriptorSets;
        std::vector<std::unique_ptr<Buffer>> m_CameraUniformBuffers, m_SceneUniformBuffers;
        std::shared_ptr<Pipeline> m_MeshPipeline, m_OutlinePipeline, m_SkyboxPipeline;
        std::vector<std::shared_ptr<DescriptorSet>> m_SceneDataDescriptorSets;
        std::shared_ptr<DescriptorSetLayout> m_SceneDescriptorSetLayout;
        VkSampler m_VkTextureSampler;

        // Shadow Map
        std::shared_ptr<RenderPass> m_ShadowMapRenderPass;
        std::shared_ptr<Pipeline> m_ShadowMapPipeline;
        std::shared_ptr<DescriptorSetLayout> m_ShadowMapDescriptorSetLayout;
        std::vector<std::shared_ptr<DescriptorSet>> m_ShadowMapDescriptorSets;
        std::vector<std::unique_ptr<Buffer>> m_ShadowMapUniformBuffers;
        VkSampler m_ShadowMapSampler;

        Cascade m_Cascades[SceneRendererSettings::CascadeCount];
        SceneRendererSettings m_RendererSettings;

        // Post processing
        std::shared_ptr<RenderPass> m_CompositePass;
        std::shared_ptr<Pipeline> m_CompositePipeline;
        std::shared_ptr<DescriptorSetLayout> m_CompositePassDescriptorSetLayout;
        std::vector<std::shared_ptr<DescriptorSet>> m_CompositePassDescriptorSets;
    };
}
