#pragma once
#include "Renderer/Framebuffer.h"
#include "Renderer/OrthographicCamera.h"

namespace Flameberry {
    class FlameEditor
    {
    public:
        FlameEditor();
        ~FlameEditor();
        void OnAttach();
        void OnDetach();
        void OnRender();
        void OnImGuiRender();
    private:
        void OnImGuiBegin();
        void OnImGuiEnd();
        void SetupImGuiStyle();
    private:
        std::shared_ptr<Framebuffer> m_Framebuffer;
        glm::vec2 m_ViewportSize;
        OrthographicCamera m_Camera;
        float m_LastRenderTime = 0.0f;
    };
}