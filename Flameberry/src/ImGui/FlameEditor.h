#pragma once
#include "../renderer/Framebuffer.h"

namespace Flameberry {
    class FlameEditor
    {
    public:
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
    };
}