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
        void OnMouseScrolledEvent(MouseScrollEvent& e);

        void InvalidateViewportImGuiDescriptorSet(uint32_t index);
        void InvalidateCompositePassImGuiDescriptorSet(uint32_t index);

        void SaveScene();
        void SaveSceneAs();
        void OpenScene();
        void OpenScene(const std::string& path);
        void NewScene();
    private:
        void UpdateShadowMapCascades();
        void ShowMenuBar();
        void CreateMousePickingPipeline();
        void CreateShadowMapPipeline();
        void CreateCompositePipeline();
    private:
        EditorCameraController m_ActiveCameraController;
        bool m_IsViewportFocused = false, m_IsViewportHovered = false;

        std::shared_ptr<DescriptorSetLayout> m_CameraBufferDescSetLayout;
        std::vector<std::shared_ptr<DescriptorSet>> m_CameraBufferDescriptorSets;

        std::unique_ptr<Buffer> m_UniformBuffers[VulkanSwapChain::MAX_FRAMES_IN_FLIGHT];

        std::unique_ptr<SceneRenderer> m_SceneRenderer;
        std::vector<std::shared_ptr<StaticMesh>> m_Meshes;

        // ECS
        std::shared_ptr<Scene> m_ActiveScene;
        std::shared_ptr<fbentt::registry> m_Registry;

        glm::vec2 m_ViewportSize{ 1280, 720 };
        glm::vec2 m_ViewportBounds[2];

        VkSampler m_VkTextureSampler;
        std::vector<VkDescriptorSet> m_ViewportDescriptorSets;
        std::vector<VkDescriptorSet> m_CompositePassViewportDescriptorSets;

        // UI
        std::shared_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
        std::shared_ptr<ContentBrowserPanel> m_ContentBrowserPanel;
        std::shared_ptr<EnvironmentSettingsPanel> m_EnvironmentSettingsPanel;

        std::string m_OpenedScenePathIfExists = "", m_ScenePathToBeOpened = "";
        bool m_ShouldOpenAnotherScene = false;

        int m_MouseX = 0, m_MouseY = 0;

        bool m_IsClickedInsideViewport = false;
        bool m_IsCameraMoving = false;
        bool m_IsGizmoActive = false;
        int m_GizmoType = -1;

        bool m_EnableGrid = true;

        std::shared_ptr<Texture2D> m_CursorIcon, m_TranslateIcon, m_RotateIcon, m_ScaleIcon;
        std::shared_ptr<Texture2D> m_CursorIconActive, m_TranslateIconActive, m_RotateIconActive, m_ScaleIconActive;

        // Debug
        glm::vec2 m_ZNearFar{ 0.5f, 50.0f };
        bool m_DisplayShadowMap = false;

        std::filesystem::path m_ProjectPath;

        std::unique_ptr<Buffer> m_MousePickingBuffer;
        bool m_IsMousePickingBufferReady = false, m_DidViewportBegin = true, m_IsGizmoOverlayHovered = false;

        std::shared_ptr<RenderPass> m_SceneRenderPass;
        std::shared_ptr<RenderPass> m_ShadowMapRenderPass;
        std::shared_ptr<RenderPass> m_MousePickingRenderPass;

        std::shared_ptr<Pipeline> m_MousePickingPipeline;
        std::shared_ptr<DescriptorSetLayout> m_MousePickingDescriptorSetLayout;

        std::shared_ptr<Pipeline> m_ShadowMapPipeline;
        VkSampler m_ShadowMapSampler;

        std::shared_ptr<DescriptorSetLayout> m_ShadowMapDescriptorSetLayout;
        std::vector<std::shared_ptr<DescriptorSet>> m_ShadowMapDescriptorSets;
        std::vector<std::unique_ptr<Buffer>> m_ShadowMapUniformBuffers;

        const uint32_t SHADOW_MAP_SIZE = 2048;
        std::array<glm::mat4, SHADOW_MAP_CASCADE_COUNT> m_CascadeMatrices;
        std::array<float, SHADOW_MAP_CASCADE_COUNT> m_CascadeDepthSplits;
        bool m_ColorCascades = false;
        float m_LambdaSplit = 0.91f;

        // Post processing resources
        std::shared_ptr<RenderPass> m_CompositePass;
        std::shared_ptr<Pipeline> m_CompositePipeline;
        std::shared_ptr<DescriptorSetLayout> m_CompositePassDescriptorSetLayout;
        std::vector<std::shared_ptr<DescriptorSet>> m_CompositePassDescriptorSets;
    };
}
