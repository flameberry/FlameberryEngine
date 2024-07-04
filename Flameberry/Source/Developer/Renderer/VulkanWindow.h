#pragma once

#include "Core/Window.h"
#include <vulkan/vulkan.h>

#include "Renderer/SwapChain.h"

namespace Flameberry {
    class VulkanWindow : public Window
    {
    public:
        VulkanWindow(const WindowSpecification& specification = WindowSpecification());
        ~VulkanWindow();

        void Init() override;
        void Shutdown() override;

        bool BeginFrame() override;
        void SwapBuffers() override;

        GLFWwindow* GetGLFWwindow() const override { return m_Window; }
        const WindowSpecification& GetSpecification() const override { return m_Specification; }
        VkSurfaceKHR GetWindowSurface() const { return m_WindowSurface; }
        uint32_t GetImageIndex() const override { return m_ImageIndex; }
        Ref<SwapChain> GetSwapChain() const { return m_SwapChain; }

        bool IsRunning() override { return !glfwWindowShouldClose(m_Window); }
        void CreateVulkanWindowSurface(VkInstance instance);
        void DestroyVulkanWindowSurface(VkInstance instance);

        void SetEventCallBack(const std::function<void(Event&)>& fn) override;
        void SetPosition(int xpos, int ypos) override;
        void SetSize(int width, int height) override;
        void SetTitle(const char* title) override;
        void SetSecondaryTitle(const char* secondaryTitle) override;
        void MoveToCenter() override;

        void Resize() override;
    private:
        GLFWwindow* m_Window;
        VkSurfaceKHR m_WindowSurface = VK_NULL_HANDLE;

        WindowSpecification m_Specification;

        uint32_t m_PrimaryMonitorWidth = 0, m_PrimaryMonitorHeight = 0;

        Ref<SwapChain> m_SwapChain;
        uint32_t m_ImageIndex = 0;
        bool m_WindowResizedFlag = false;

        std::function<void(Event&)> m_EventCallBack;
    };
}
