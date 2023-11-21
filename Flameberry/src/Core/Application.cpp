#include "Application.h"

#include "Core.h"
#include "Layer.h"
#include "Timer.h"

#include "ImGui/ImGuiLayer.h"
#include "Renderer/Renderer.h"
#include "Renderer/Texture2D.h"
#include "Asset/AssetManager.h"

// TODO: Find a better place for this
#include "Physics/PhysicsEngine.h"

#include "Platform/PlatformUtils.h"

#ifdef FL_DEBUG
#define FL_WINDOW_TITLE "Flameberry Engine [Debug]"
#elif defined(FL_RELEASE)
#define FL_WINDOW_TITLE "Flameberry Engine [Release]"
#else
#define FL_WINDOW_TITLE "Flameberry Engine [Unknown]"
#endif

namespace Flameberry {
    Application* Application::s_Instance;

    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create(1280, 720, FL_WINDOW_TITLE);
        m_Window->SetEventCallBack(FL_BIND_EVENT_FN(Application::OnEvent));

        m_VulkanContext = VulkanContext::Create((VulkanWindow*)m_Window.get());
        VulkanContext::SetCurrentContext(m_VulkanContext.get());

        m_Window->Init();

        auto swapchain = VulkanContext::GetCurrentWindow()->GetSwapChain();
        VulkanContext::GetCurrentDevice()->AllocateCommandBuffers(swapchain->GetSwapChainImageCount());

        // Create the generic texture descriptor layout
        Texture2D::InitStaticResources();

        m_ImGuiLayer = std::make_unique<ImGuiLayer>();

        PhysicsEngine::Init();
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
            layer->OnEvent(e);
            if (e.Handled)
                break;
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

    Application::~Application()
    {
        VulkanContext::GetCurrentDevice()->WaitIdle();
        for (auto& layer : m_LayerStack)
            layer->OnDestroy();

        PhysicsEngine::Shutdown();
        m_ImGuiLayer->OnDestroy();

        Texture2D::DestroyStaticResources();
        AssetManager::Clear();

        m_Window->Shutdown();

        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}
