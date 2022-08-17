#pragma once
#include "Flameberry.h"

class FlameEditorApp : public Flameberry::Application
{
public:
    FlameEditorApp();
    virtual ~FlameEditorApp();
    void OnRender() override;
    void OnUIRender() override;
private:
    std::shared_ptr<Flameberry::Framebuffer> m_Framebuffer;
    glm::vec2 m_ViewportSize;
    Flameberry::OrthographicCamera m_Camera;
    float m_LastRenderTime = 0.0f;
};