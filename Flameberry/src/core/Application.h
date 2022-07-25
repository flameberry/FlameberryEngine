#pragma once
#include <memory>
#include "Window.h"
#include "../ImGui/FlameEditor.h"

namespace Flameberry {
    class Application
    {
    public:
        Application();
        ~Application();
        void Run();

        Window& GetWindow() { return *M_Window; }
        static Application& Get() { return *S_Instance; }
    private:
        std::shared_ptr<Window> M_Window;
        FlameEditor M_FlameEditor;
    private:
        static Application* S_Instance;
    };
}