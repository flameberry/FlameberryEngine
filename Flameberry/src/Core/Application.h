#pragma once

#include <memory>

#include "Window.h"
#include "Renderer/Renderer3D.h"

namespace Flameberry {
    class Application
    {
    public:
        Application();
        ~Application();
        void Run();

        Window& GetWindow() { return *m_Window; }
        static Application& Get() { return *s_Instance; }
        static std::shared_ptr<Application> CreateClientApp();

        virtual void OnUpdate(float delta) = 0;
        virtual void OnUIRender() = 0;
    private:
        std::shared_ptr<Window> m_Window;
        std::shared_ptr<Renderer3D> m_Renderer3D;
        PerspectiveCamera m_Camera;
    private:
        static Application* s_Instance;
    };
}