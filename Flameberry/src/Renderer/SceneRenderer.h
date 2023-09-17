#pragma once

#include "Pipeline.h"
#include "SwapChain.h"
#include "StaticMesh.h"
#include "DescriptorSet.h"
#include "CommandBuffer.h"

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
        static const uint32_t CascadeCount = 4, CascadeSize = 1028 * 2; // TODO: Make this a renderer startup setting
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

        void RenderScene(const glm::vec2& viewportSize, const std::shared_ptr<Scene>& scene, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& cameraPosition, float cameraNear, float cameraFar, fbentt::entity selectedEntity, bool renderGrid = true, bool renderDebugIcons = true, bool renderOutline = true, bool renderPhysicsCollider = true);
        void RenderSceneRuntime(const glm::vec2& viewportSize, const std::shared_ptr<Scene>& scene, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& cameraPosition, float cameraNear, float cameraFar);

        VkImageView GetGeometryPassOutputImageView(uint32_t index) const { return m_GeometryPass->GetSpecification().TargetFramebuffers[index]->GetColorResolveAttachment(0)->GetImageView(); }
        VkImageView GetCompositePassOutputImageView(uint32_t index) const { return m_CompositePass->GetSpecification().TargetFramebuffers[index]->GetColorAttachment(0)->GetImageView(); }

        SceneRendererSettings& GetRendererSettingsRef() { return m_RendererSettings; }
        void RenderSceneForMousePicking(const std::shared_ptr<Scene>& scene, const std::shared_ptr<RenderPass>& renderPass, const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Pipeline>& pipeline2D, const glm::vec2& mousePos);

        void ReloadMeshShaders();
    private:
        void Init();

        void CalculateShadowMapCascades(const glm::mat4& viewProjectionMatrix, float cameraNear, float cameraFar, const glm::vec3& lightDirection);
        void SubmitMesh(AssetHandle handle, const MaterialTable& materialTable, const glm::mat4& transform);
        void SubmitPhysicsColliderGeometry(const std::shared_ptr<Scene>& scene, fbentt::entity entity, TransformComponent& transform);
        void SubmitCameraViewGeometry(const std::shared_ptr<Scene>& scene, fbentt::entity entity, TransformComponent& transform);
    private:
        glm::vec2 m_ViewportSize;

        // Command Buffers
        std::vector<std::shared_ptr<CommandBuffer>> m_CommandBuffers;

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

        // Textures
        std::shared_ptr<Texture2D> m_LightIcon, m_CameraIcon;
    };
}
