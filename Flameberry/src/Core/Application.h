#pragma once

#include <memory>

#include "Window.h"
#include "Layer.h"

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

        void OnEvent(Event& e);
        void OnKeyPressedEvent(KeyPressedEvent& e);

        template<typename T> void PushLayer() {
            auto& layer = m_LayerStack.emplace_back(std::make_shared<T>());
            layer->OnCreate();
        }
        inline void PopLayer() { m_LayerStack.pop_back(); }
    private:
        std::shared_ptr<Window> m_Window;
        std::vector<std::shared_ptr<Layer>> m_LayerStack;
    private:
        static Application* s_Instance;
    };
}
