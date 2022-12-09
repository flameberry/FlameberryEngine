#pragma once

#include "Flameberry.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Renderer/Light.h"

namespace Flameberry {
    class FlameEditorApp: public Flameberry::Application
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
        std::shared_ptr<Flameberry::OpenGLFramebuffer> m_Framebuffer;
        glm::vec2 m_ViewportSize;
        Flameberry::OrthographicCamera m_Camera;
        Flameberry::PerspectiveCamera m_PerspectiveCamera;
        double m_LastRenderTime = 0.0;
        glm::vec2 m_ViewportBounds[2];
        std::shared_ptr<Flameberry::OpenGLRenderer3D> m_Renderer3D;
        bool m_IsCursorInsideViewport = false;

        Flameberry::Mesh m_TempMesh, m_SponzaMesh, m_FloorMesh;
        std::vector<Flameberry::PointLight> m_PointLights;
        DirectionalLight m_DirectionalLight;
    private:
        SceneHierarchyPanel m_SceneHierarchyPanel;
        ContentBrowserPanel m_ContentBrowserPanel;
        // ECS system objects
        std::shared_ptr<Flameberry::Scene> m_ActiveScene;
        std::shared_ptr<Flameberry::Registry> m_Registry;
        Flameberry::entity_handle m_SquareEntity, m_TexturedEntity, m_BlueSquareEntity;
    };
}