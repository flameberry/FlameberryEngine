#pragma once

#include <memory>

#include "Window.h"
#include "Layer.h"
#include "ImGui/ImGuiLayer.h"

#include "Renderer/VulkanContext.h"

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
        void OnWindowResizedEvent(WindowResizedEvent& e);

        template<typename T, typename... Args> void PushLayer(Args... args) {
            auto& layer = m_LayerStack.emplace_back(std::make_shared<T>(std::forward<Args>(args)...));
            layer->OnCreate();
        }
        inline void PopLayer() { m_LayerStack.pop_back(); }
    private:
        std::shared_ptr<Window> m_Window;
        std::shared_ptr<VulkanContext> m_VulkanContext;
        std::unique_ptr<ImGuiLayer> m_ImGuiLayer;

        std::vector<std::shared_ptr<Layer>> m_LayerStack;
    private:
        static Application* s_Instance;
    };
}
