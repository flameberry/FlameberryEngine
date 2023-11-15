#include "VulkanWindow.h"

#include "Renderer/VulkanDebug.h"
#include "Renderer/VulkanContext.h"

#include "Renderer/Renderer.h"

namespace Flameberry {
    std::shared_ptr<Window> Window::Create(int width, int height, const char* title)
    {
        return std::make_shared<VulkanWindow>(width, height, title);
    }

    VulkanWindow::VulkanWindow(int width, int height, const char* title)
        : m_Width(width), m_Height(height), m_Title(title)
    {
        FL_ASSERT(glfwInit(), "Failed to initialize GLFW!");
        FL_INFO("Initialized GLFW!");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_Window = glfwCreateWindow(m_Width, m_Height, m_Title, NULL, NULL);
        FL_ASSERT(m_Window, "GLFW window is null!");
        FL_INFO("Created GLFW window of title '{0}' and dimensions ({1}, {2})", m_Title, m_Width, m_Height);
    }

    void VulkanWindow::Init()
    {
        m_SwapChain = std::make_shared<SwapChain>(m_WindowSurface);
    }

    void VulkanWindow::Shutdown()
    {
        m_SwapChain = nullptr;
        glfwDestroyWindow(m_Window);
    }

    void VulkanWindow::SetEventCallBack(const std::function<void(Event&)>& fn)
    {
        m_EventCallBack = fn;
        glfwSetWindowUserPointer(m_Window, this);

        glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
            {
                VulkanWindow* pWindow = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
                pWindow->m_Width = width;
                pWindow->m_Height = height;
            
                WindowResizedEvent event(width, height);
                pWindow->m_EventCallBack(event);
            });

        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                VulkanWindow* ptr = (VulkanWindow*)glfwGetWindowUserPointer(window);
                if (action == GLFW_PRESS)
                {
                    KeyPressedEvent event(key);
                    ptr->m_EventCallBack(event);
                }
            }
        );

        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xoffset, double yoffset)
            {
                VulkanWindow* ptr = (VulkanWindow*)glfwGetWindowUserPointer(window);
                MouseScrollEvent event(xoffset, yoffset);
                ptr->m_EventCallBack(event);
            }
        );
    }

    void VulkanWindow::CreateVulkanWindowSurface(VkInstance instance)
    {
        VK_CHECK_RESULT(glfwCreateWindowSurface(instance, m_Window, nullptr, &m_WindowSurface));
    }

    void VulkanWindow::DestroyVulkanWindowSurface(VkInstance instance)
    {
        vkDestroySurfaceKHR(instance, m_WindowSurface, nullptr);
    }

    VulkanWindow::~VulkanWindow()
    {
    }

    bool VulkanWindow::BeginFrame()
    {
        VkResult result = m_SwapChain->AcquireNextImage();
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_SwapChain->Invalidate();
            return false;
        }
        m_ImageIndex = m_SwapChain->GetAcquiredImageIndex();
        return true;
    }

    void VulkanWindow::SwapBuffers()
    {
        const auto& device = VulkanContext::GetCurrentDevice();
        VkResult queuePresentStatus = m_SwapChain->SubmitCommandBuffer(device->GetCommandBuffer(Renderer::RT_GetCurrentFrameIndex()));
        
        // TODO: This code should be enabled when ensured that all the resources that depend upon the swapchain are also updated
//        if (queuePresentStatus == VK_ERROR_OUT_OF_DATE_KHR || queuePresentStatus == VK_SUBOPTIMAL_KHR)
//            m_SwapChain->Invalidate();
    }
    
    void VulkanWindow::Resize() 
    {
        m_SwapChain->Invalidate();
    }
    
}
