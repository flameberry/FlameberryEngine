#pragma once

#include "Flameberry.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Renderer/Light.h"

namespace Flameberry {
    class FlameEditorApp: public Application
    {
    public:
        FlameEditorApp();
        virtual ~FlameEditorApp();
        void OnUpdate(float delta) override;
        void OnUIRender() override;
        void SaveScene();
        void OpenScene();
        void SaveScene(const std::string& path);
        void OpenScene(const std::string& path);
    private:
        std::shared_ptr<OpenGLFramebuffer> m_Framebuffer;
        glm::vec2 m_ViewportSize;
        PerspectiveCamera m_EditorCamera;
        glm::vec2 m_ViewportBounds[2];

        Mesh m_TempMesh, m_SponzaMesh, m_FloorMesh;
        std::shared_ptr<OpenGLTexture> m_BrickTexture;

        DirectionalLight m_DirectionalLight;
        std::shared_ptr<SceneRenderer> m_SceneRenderer;
    private:
        // Shadow Mapping Resources
        std::shared_ptr<OpenGLFramebuffer> m_ShadowMapFramebuffer;
        std::shared_ptr<OpenGLShader> m_ShadowMapShader;
        OpenGLBuffer m_ShadowMapUniformBuffer;
    private:
        SceneHierarchyPanel m_SceneHierarchyPanel;
        ContentBrowserPanel m_ContentBrowserPanel;
        // ECS system objects
        std::shared_ptr<Scene> m_ActiveScene;
        std::shared_ptr<ecs::registry> m_Registry;
        ecs::entity_handle m_SquareEntity, m_TexturedEntity, m_BlueSquareEntity;

        bool m_IsGizmoActive = false;
        int m_GizmoType = -1;
    };
}