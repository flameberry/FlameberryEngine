#include "VulkanWindow.h"

#include "Renderer/Vulkan/VulkanDebug.h"
#include "Renderer/Vulkan/VulkanContext.h"

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

        glfwSetWindowUserPointer(m_Window, this);
        glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallBack);

    }

    void VulkanWindow::SetEventCallBack(const std::function<void(Event&)>& fn)
    {
        m_EventCallBack = fn;
        glfwSetWindowUserPointer(m_Window, this);

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
    }

    void VulkanWindow::FramebufferResizeCallBack(GLFWwindow* window, int width, int height)
    {
        VulkanWindow* pWindow = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        pWindow->m_Width = width;
        pWindow->m_Height = height;
        pWindow->m_FramebufferResized = true;
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
        glfwDestroyWindow(m_Window);
    }

    void VulkanWindow::OnUpdate()
    {
        glfwPollEvents();
    }
}
