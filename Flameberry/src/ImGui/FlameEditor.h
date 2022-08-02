#pragma once
#include "../renderer/Framebuffer.h"
#include "../renderer/OrthographicCamera.h"

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
        void OnImGuiBegin();
        void OnImGuiEnd();
    private:
        void SetupImGuiStyle();
    private:
        std::shared_ptr<Framebuffer> M_Framebuffer;
        glm::vec2 M_ViewportSize;
        OrthographicCamera M_Camera;
    };
}