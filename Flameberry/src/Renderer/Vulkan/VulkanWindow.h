#pragma once

#include "Core/Window.h"
#include <vulkan/vulkan.h>

namespace Flameberry {
    class VulkanWindow: public Window
    {
    public:
        VulkanWindow(int width = 1280, int height = 720, const char* title = "Flameberry Engine");
        ~VulkanWindow();

        GLFWwindow* GetGLFWwindow() const override { return m_Window; }
        VkSurfaceKHR GetWindowSurface() const { return m_WindowSurface; }
        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }

        bool IsRunning() override { return !glfwWindowShouldClose(m_Window); }
        void OnUpdate() override;
        void CreateVulkanWindowSurface(VkInstance instance);
        void DestroyVulkanWindowSurface(VkInstance instance);
    private:
        GLFWwindow* m_Window;
        VkSurfaceKHR m_WindowSurface = VK_NULL_HANDLE;
        uint32_t m_Width, m_Height;
        const char* m_Title;
    };
}