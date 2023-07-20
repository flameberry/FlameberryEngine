#include "Application.h"

#include "Timer.h"
#include "Core.h"
#include "Layer.h"

#include "ImGui/ImGuiLayer.h"
#include "Renderer/Renderer.h"
#include "Renderer/Texture2D.h"
#include "Asset/AssetManager.h"

#include "Platform/PlatformUtils.h"

// TODO: Find a better place for this
#include "Physics/Physics.h"

namespace Flameberry {
    Application* Application::s_Instance;

    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();
        m_Window->SetEventCallBack(FL_BIND_EVENT_FN(Application::OnEvent));

        m_VulkanContext = VulkanContext::Create((VulkanWindow*)m_Window.get());
        VulkanContext::SetCurrentContext(m_VulkanContext.get());

        m_Window->Init();

        auto swapchain = VulkanContext::GetCurrentWindow()->GetSwapChain();
        VulkanContext::GetCurrentDevice()->AllocateCommandBuffers(swapchain->GetSwapChainImageCount());

        // Create the generic texture descriptor layout
        Texture2D::InitStaticResources();

        m_ImGuiLayer = std::make_unique<ImGuiLayer>();
        
        PhysicsContext::Init();
    }

    void Application::Run()
    {
        float last = 0.0f;
        while (m_Window->IsRunning())
        {
            float now = glfwGetTime();
            float delta = now - last;
            last = now;

            for (auto& layer : m_LayerStack)
                layer->OnUpdate(delta);

            if (m_Window->BeginFrame())
            {
                Renderer::Render();

                m_ImGuiLayer->Begin();
                for (auto& layer : m_LayerStack)
                    layer->OnUIRender();
                m_ImGuiLayer->End();

                m_Window->SwapBuffers();
            }
            
            if (m_Window->IsWindowResized())
            {
                m_ImGuiLayer->InvalidateResources();
                m_Window->ResetWindowResizedFlag();
            }

            Renderer::ClearCommandQueue();
            glfwPollEvents();
        }
    }

    void Application::OnEvent(Event& e)
    {
        switch (e.GetType()) {
            case EventType::KEY_PRESSED:
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

    Application::~Application()
    {
        VulkanContext::GetCurrentDevice()->WaitIdle();
        for (auto& layer : m_LayerStack)
            layer->OnDestroy();

        PhysicsContext::Release();
        m_ImGuiLayer->OnDestroy();

        Texture2D::DestroyStaticResources();
        AssetManager::Clear();

        m_Window->Destroy();

        glfwTerminate();
        FL_INFO("Ended Application!");
    }
}
