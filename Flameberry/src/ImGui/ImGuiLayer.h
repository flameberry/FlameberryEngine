#pragma once

namespace Flameberry {
    class ImGuiLayer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();
        static void OnAttach();
        static void OnDetach();
        static void Begin();
        static void End();
    private:
        static void SetupImGuiStyle();
    };
}