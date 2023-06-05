#pragma once

#include "Flameberry.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/EnvironmentSettingsPanel.h"

namespace Flameberry {
    class EditorLayer : public Layer
    {
    public:
        EditorLayer(const std::string_view& projectPath);
        virtual ~EditorLayer() = default;

        void OnCreate() override;
        void OnUpdate(float delta) override;
        void OnUIRender() override;
        void OnEvent(Event& e) override;
        void OnDestroy() override;

        void OnKeyPressedEvent(KeyPressedEvent& e);
        void OnMouseButtonPressedEvent(MouseButtonPressedEvent& e);

        void InvalidateViewportImGuiDescriptorSet(uint32_t index);
        void InvalidateShadowMapImGuiDescriptorSet();

        void SaveScene();
        void OpenScene();
        void SaveScene(const std::string& path);
        void OpenScene(const std::string& path);
    private:
        void ShowMenuBar();
        void CreateMousePickingPipeline();
        void CreateShadowMapPipeline();
    private:
        std::shared_ptr<VulkanRenderer> m_VulkanRenderer;

        PerspectiveCamera m_ActiveCamera;
        bool m_IsViewportFocused = false;

        std::shared_ptr<DescriptorSetLayout> m_CameraBufferDescSetLayout;
        std::vector<std::shared_ptr<DescriptorSet>> m_CameraBufferDescriptorSets;

        std::unique_ptr<VulkanBuffer> m_UniformBuffers[VulkanSwapChain::MAX_FRAMES_IN_FLIGHT];

        std::unique_ptr<SceneRenderer> m_SceneRenderer;
        std::vector<std::shared_ptr<StaticMesh>> m_Meshes;

        // std::unique_ptr<SkyboxRenderer> m_SkyboxRenderer;

        // ECS
        std::shared_ptr<Scene> m_ActiveScene;
        std::shared_ptr<ecs::registry> m_Registry;

        std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
        glm::vec2 m_ViewportSize{ 1280, 720 };
        glm::vec2 m_ViewportBounds[2];
        glm::vec2 m_ShadowMapViewportSize{ 1280, 720 };

        VkSampler m_VkTextureSampler;
        std::vector<VkDescriptorSet> m_ViewportDescriptorSets;
        std::vector<VkDescriptorSet> m_ShadowMapViewportDescriptorSets;

        // UI
        std::shared_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
        std::shared_ptr<ContentBrowserPanel> m_ContentBrowserPanel;
        std::shared_ptr<EnvironmentSettingsPanel> m_EnvironmentSettingsPanel;

        std::string m_OpenedScenePathIfExists = "", m_ScenePathToBeOpened = "";
        bool m_ShouldOpenAnotherScene = false;

        bool m_IsCameraMoving = false;
        bool m_IsGizmoActive = false;
        int m_GizmoType = -1;

        std::shared_ptr<VulkanTexture> m_CursorIcon, m_TranslateIcon, m_RotateIcon, m_ScaleIcon;
        std::shared_ptr<VulkanTexture> m_CursorIconActive, m_TranslateIconActive, m_RotateIconActive, m_ScaleIconActive;

        // Debug
        glm::vec2 m_ZNearFar{ 0.5f, 50.0f };
        bool m_DisplayShadowMap = false;

        std::filesystem::path m_ProjectPath;

        std::unique_ptr<VulkanBuffer> m_MousePickingBuffer;
        bool m_IsClickedInsideViewport = false, m_DidViewportBegin = true, m_IsGizmoOverlayHovered = false;

        // Test
        std::shared_ptr<RenderPass> m_SceneRenderPass;
        std::vector<std::shared_ptr<Framebuffer>> m_SceneFramebuffers;

        std::shared_ptr<RenderPass> m_ShadowMapRenderPass;
        std::vector<std::shared_ptr<Framebuffer>> m_ShadowMapFramebuffers;

        std::shared_ptr<RenderPass> m_MousePickingRenderPass;
        std::shared_ptr<Framebuffer> m_MousePickingFramebuffer;

        VkPipelineLayout m_MousePickingPipelineLayout;
        std::shared_ptr<Pipeline> m_MousePickingPipeline;

        std::shared_ptr<DescriptorSetLayout> m_MousePickingDescriptorSetLayout;
        std::shared_ptr<DescriptorSet> m_MousePickingDescriptorSet;
        std::unique_ptr<VulkanBuffer> m_MousePickingUniformBuffer;

        std::shared_ptr<Pipeline> m_ShadowMapPipeline;
        VkPipelineLayout m_ShadowMapPipelineLayout;

        VkSampler m_ShadowMapSampler;

        std::shared_ptr<DescriptorSetLayout> m_ShadowMapDescriptorSetLayout;
        std::vector<std::shared_ptr<DescriptorSet>> m_ShadowMapDescriptorSets;
        std::vector<std::unique_ptr<VulkanBuffer>> m_ShadowMapUniformBuffers;

        const uint32_t SHADOW_MAP_WIDTH = 2048, SHADOW_MAP_HEIGHT = 2048;
    };
}