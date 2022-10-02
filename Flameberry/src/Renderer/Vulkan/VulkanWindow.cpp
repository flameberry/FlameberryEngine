#include "VulkanWindow.h"

#include "Core/Core.h"

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

    VulkanWindow::~VulkanWindow()
    {
        glfwDestroyWindow(m_Window);
    }

    void VulkanWindow::OnUpdate()
    {
        glfwPollEvents();
    }
}