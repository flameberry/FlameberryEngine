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

        void ShowMenuBar();
    private:
        EditorCameraController m_ActiveCameraController;

        // Scalars
        int m_MouseX = 0, m_MouseY = 0;
        int m_GizmoType = -1;
        bool m_IsViewportFocused = false, m_IsViewportHovered = false, m_IsClickedInsideViewport = false;
        bool m_IsCameraMoving = false, m_IsGizmoActive = false;
        bool m_IsMousePickingBufferReady = false, m_DidViewportBegin = true, m_IsGizmoOverlayHovered = false;
        bool m_EnableGrid = true;

        glm::vec2 m_ViewportSize{ 1280, 720 };
        glm::vec2 m_ViewportBounds[2];

        std::string m_OpenedScenePathIfExists = "", m_ScenePathToBeOpened = "";
        bool m_ShouldOpenAnotherScene = false;
        std::filesystem::path m_ProjectPath;

        // Texture Icons
        std::shared_ptr<Texture2D> m_CursorIcon, m_TranslateIcon, m_RotateIcon, m_ScaleIcon;
        std::shared_ptr<Texture2D> m_CursorIconActive, m_TranslateIconActive, m_RotateIconActive, m_ScaleIconActive;

        // Renderer
        std::unique_ptr<SceneRenderer> m_SceneRenderer;

        // ECS
        std::shared_ptr<Scene> m_ActiveScene;
        std::shared_ptr<fbentt::registry> m_Registry;


        std::vector<VkDescriptorSet> m_ViewportDescriptorSets;
        std::vector<VkDescriptorSet> m_CompositePassViewportDescriptorSets;

        // UI
        std::shared_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
        std::shared_ptr<ContentBrowserPanel> m_ContentBrowserPanel;
        std::shared_ptr<EnvironmentSettingsPanel> m_EnvironmentSettingsPanel;

        // Mouse Picking
        std::unique_ptr<Buffer> m_MousePickingBuffer;
        std::shared_ptr<RenderPass> m_MousePickingRenderPass;

        std::shared_ptr<Pipeline> m_MousePickingPipeline;
        std::shared_ptr<DescriptorSetLayout> m_MousePickingDescriptorSetLayout;
    };
}
