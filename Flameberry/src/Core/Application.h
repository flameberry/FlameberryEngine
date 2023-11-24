#pragma once

#include <memory>

#include "Window.h"
#include "Layer.h"
#include "ImGui/ImGuiLayer.h"

#include "Renderer/VulkanContext.h"

namespace Flameberry {
    
    struct ApplicationSpecification
    {
        std::string Name;
        WindowSpecification WindowSpec;
        std::filesystem::path WorkingDirectory;
    };
    
    class Application
    {
    public:
        Application(const ApplicationSpecification& specification);
        ~Application();
        void Run();

        Window& GetWindow() { return *m_Window; }
        static Application& Get() { return *s_Instance; }
        static Application* CreateClientApp();

        void OnEvent(Event& e);
        void OnKeyPressedEvent(KeyPressedEvent& e);
        void OnWindowResizedEvent(WindowResizedEvent& e);
        
        void BlockImGuiEvents(bool value) { m_ImGuiLayer->BlockEvents(value); }

        void PushLayer(Layer* layer);
        void PopLayer(Layer* layer);
    private:
        ApplicationSpecification m_Specification;
        
        std::shared_ptr<Window> m_Window;
        std::shared_ptr<VulkanContext> m_VulkanContext;
        std::unique_ptr<ImGuiLayer> m_ImGuiLayer;

        std::vector<Layer*> m_LayerStack;
    private:
        static Application* s_Instance;
    };
}
