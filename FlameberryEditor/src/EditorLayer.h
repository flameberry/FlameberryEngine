#pragma once

#include "Flameberry.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

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

        void InvalidateViewportImGuiDescriptorSet();
        void InvalidateShadowMapImGuiDescriptorSet();

        void SaveScene();
        void OpenScene();
        void SaveScene(const std::string& path);
        void OpenScene(const std::string& path);
    private:
        std::shared_ptr<VulkanRenderer> m_VulkanRenderer;

        PerspectiveCamera m_ActiveCamera;
        bool m_IsViewportFocused = false;

        std::unique_ptr<VulkanDescriptorWriter> m_VulkanDescriptorWriter;
        std::unique_ptr<VulkanDescriptorLayout> m_VulkanDescriptorLayout;

        std::vector<VkDescriptorSet> m_VkDescriptorSets;

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
        std::vector<VkDescriptorSet> m_ShadowMapDescriptorSets;

        // UI
        std::shared_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
        std::shared_ptr<ContentBrowserPanel> m_ContentBrowserPanel;

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
        bool m_IsClickedInsideViewport = false;
        // std::shared_ptr<Material> m_CurrentMaterial;

        // Test
        std::shared_ptr<RenderPass> m_SceneRenderPass;
        std::vector<std::shared_ptr<Framebuffer>> m_SceneFramebuffers;

        std::shared_ptr<RenderPass> m_ShadowMapRenderPass;
        std::vector<std::shared_ptr<Framebuffer>> m_ShadowMapFramebuffers;

        std::shared_ptr<RenderPass> m_MousePickingRenderPass;
        std::shared_ptr<Framebuffer> m_MousePickingFramebuffer;
    };
}