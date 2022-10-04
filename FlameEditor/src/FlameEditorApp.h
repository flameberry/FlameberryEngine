#pragma once

#include "Flameberry.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

class FlameEditorApp : public Flameberry::Application
{
public:
    FlameEditorApp();
    virtual ~FlameEditorApp();
    void OnUpdate(float delta) override;
    void OnUIRender() override;
private:
    std::shared_ptr<Flameberry::OpenGLFramebuffer> m_Framebuffer;
    glm::vec2 m_ViewportSize;
    Flameberry::OrthographicCamera m_Camera;
    Flameberry::PerspectiveCamera m_PerspectiveCamera;
    double m_LastRenderTime = 0.0;
    glm::vec2 m_ViewportBounds[2];
    // std::shared_ptr<Flameberry::OpenGLRenderer3D> m_Renderer3D;
    std::shared_ptr<Flameberry::OpenGLRenderer2D> m_Renderer2D;
private:
    SceneHierarchyPanel m_SceneHierarchyPanel;
    ContentBrowserPanel m_ContentBrowserPanel;
    // ECS system objects
    std::shared_ptr<Flameberry::Scene> m_Scene;
    std::shared_ptr<Flameberry::Registry> m_Registry;
    Flameberry::entity_handle m_SquareEntity, m_TexturedEntity, m_BlueSquareEntity;
};