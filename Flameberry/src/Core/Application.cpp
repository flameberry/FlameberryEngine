#include "Application.h"

#include "Core.h"
#include "Layer.h"
#include "Timer.h"

#include "ImGui/ImGuiLayer.h"
#include "Renderer/Renderer.h"
#include "Renderer/Texture2D.h"
#include "Asset/AssetManager.h"

#include "Platform/PlatformUtils.h"

namespace Flameberry {
    Application* Application::s_Instance;

    Application::Application(const ApplicationSpecification& specification)
        : m_Specification(specification)
    {
        s_Instance = this;
        m_Window = Window::Create(m_Specification.WindowSpec);
        m_Window->SetEventCallBack(FBY_BIND_EVENT_FN(Application::OnEvent));

        m_VulkanContext = VulkanContext::Create((VulkanWindow*)m_Window.get());
        VulkanContext::SetCurrentContext(m_VulkanContext.get());

        m_Window->Init();

        auto swapchain = VulkanContext::GetCurrentWindow()->GetSwapChain();
        VulkanContext::GetCurrentDevice()->AllocateCommandBuffers(swapchain->GetSwapChainImageCount());

        // Create the generic texture descriptor layout
        Texture2D::InitStaticResources();

        m_ImGuiLayer = std::make_unique<ImGuiLayer>();
    }

    void Application::Run()
    {
        float last = 0.0f;

        Renderer::Init();
        while (m_Window->IsRunning())
        {
            float now = glfwGetTime();
            float delta = now - last;
            last = now;

            for (auto& layer : m_LayerStack)
                layer->OnUpdate(delta);

            Renderer::Submit([app = this](VkCommandBuffer, uint32_t)
                {
                    app->m_ImGuiLayer->Begin();
                    for (auto& layer : app->m_LayerStack)
                        layer->OnUIRender();
                    app->m_ImGuiLayer->End();
                });

            Renderer::WaitAndRender();
            glfwPollEvents();
        }
        Renderer::Shutdown();
    }

    void Application::OnEvent(Event& e)
    {
        switch (e.GetType()) {
            case EventType::WindowResized:
                this->OnWindowResizedEvent(*(WindowResizedEvent*)(&e));
                break;
            case EventType::KeyPressed:
                this->OnKeyPressedEvent(*(KeyPressedEvent*)(&e));
                break;
        }

        m_ImGuiLayer->OnEvent(e);
        for (auto& layer : m_LayerStack)
        {
            if (e.Handled)
                break;
            layer->OnEvent(e);
        }
    }

    void Application::OnKeyPressedEvent(KeyPressedEvent& e)
    {
    }

    void Application::OnWindowResizedEvent(WindowResizedEvent& e)
    {
        // TODO: Wait for Render Thread to finish Rendering
        m_Window->Resize();
        m_ImGuiLayer->InvalidateResources();
    }
    
    void Application::PushLayer(Layer* layer) 
    {
        m_LayerStack.emplace_back(layer);
        layer->OnCreate();
    }
    
    void Application::PopLayer(Layer* layer) 
    {
        auto iterator = std::find(m_LayerStack.begin(), m_LayerStack.end(), layer);
        (*iterator)->OnDestroy();
        delete (*iterator);
        m_LayerStack.erase(iterator);
    }

    Application::~Application()
    {
        VulkanContext::GetCurrentDevice()->WaitIdle();
        
        for (auto* layer : m_LayerStack)
        {
            layer->OnDestroy();
            delete layer;
        }
        
        m_ImGuiLayer->OnDestroy();

        Texture2D::DestroyStaticResources();
        AssetManager::Clear();

        m_Window->Shutdown();

        glfwTerminate();
        FBY_INFO("Ended Application!");
    }
}
