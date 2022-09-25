#pragma once

#include "Flameberry.h"
#include "Panels/Panel.h"

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
    // std::shared_ptr<Flameberry::OpenGLRenderer3D> m_Renderer3D;
    std::shared_ptr<Flameberry::OpenGLRenderer2D> m_Renderer2D;
private:
    std::vector<std::shared_ptr<Panel>> m_Panels;

    // ECS system objects
    std::shared_ptr<Flameberry::Scene> m_Scene;
    std::shared_ptr<Flameberry::Registry> m_Registry;
    Flameberry::entity_handle m_SquareEntity, m_TexturedEntity, m_BlueSquareEntity;
};