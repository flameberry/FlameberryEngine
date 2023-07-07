#pragma once

#include "Core/Window.h"
#include <vulkan/vulkan.h>

#include "Renderer/Vulkan/VulkanSwapChain.h"

namespace Flameberry {
    class VulkanWindow : public Window
    {
    public:
        VulkanWindow(int width = 1280, int height = 720, const char* title = "Flameberry Engine");
        ~VulkanWindow();

        void Init() override;
        void Destroy() override;

        bool BeginFrame() override;
        void SwapBuffers() override;

        GLFWwindow* GetGLFWwindow() const override { return m_Window; }
        VkSurfaceKHR GetWindowSurface() const { return m_WindowSurface; }
        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }
        uint32_t GetImageIndex() const override { return m_ImageIndex; }
        uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
        std::shared_ptr<SwapChain> GetSwapChain() const { return m_SwapChain; }

        bool IsRunning() override { return !glfwWindowShouldClose(m_Window); }
        void CreateVulkanWindowSurface(VkInstance instance);
        void DestroyVulkanWindowSurface(VkInstance instance);

        bool IsWindowResized() const override { return m_FramebufferResized; }
        void ResetWindowResizedFlag() override { m_FramebufferResized = false; }
        void SetEventCallBack(const std::function<void(Event&)>& fn) override;
    private:
        static void FramebufferResizeCallBack(GLFWwindow* window, int width, int height);
    private:
        GLFWwindow* m_Window;
        VkSurfaceKHR m_WindowSurface = VK_NULL_HANDLE;
        uint32_t m_Width, m_Height;
        const char* m_Title;

        std::shared_ptr<SwapChain> m_SwapChain;
        uint32_t m_CurrentFrameIndex = 0, m_ImageIndex = 0;

        std::function<void(Event&)> m_EventCallBack;
        bool m_FramebufferResized = false;
    };
}