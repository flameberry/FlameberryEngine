#pragma once

#include <memory>

#include "Window.h"

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

        // virtual void OnKeyPress(uint32_t key);
    private:
        std::shared_ptr<Window> m_Window;
    private:
        static Application* s_Instance;
    };
}