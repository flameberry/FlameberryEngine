#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include "VulkanImage.h"

namespace Flameberry {
    class VulkanSwapChain
    {
    public:
        VulkanSwapChain(VkSurfaceKHR surface);
        ~VulkanSwapChain();

        VkRenderPass GetRenderPass() const { return m_VkRenderPass; }
        VkExtent2D GetExtent2D() const { return m_VkSwapChainExtent2D; }
        uint32_t GetAcquiredImageIndex() const { return m_ImageIndex; }
        VkFramebuffer GetFramebuffer(uint32_t index) { return m_VkSwapChainFramebuffers[index]; }
        uint32_t GetSwapChainImageCount() const { return m_VkSwapChainImages.size(); }

        VkResult AcquireNextImage();
        VkResult SubmitCommandBuffer(VkCommandBuffer commandBuffer);
        void Invalidate();
    private:
        VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
        VkPresentModeKHR SelectSwapPresentationMode(const std::vector<VkPresentModeKHR>& available_presentation_modes);
        VkExtent2D SelectSwapExtent(const VkSurfaceCapabilitiesKHR& surface_capabilities);
        VkFormat GetDepthFormat();
        void CreateRenderPass();
        void CreateSyncObjects();
    private:
        VkSwapchainKHR m_VkSwapChain;
        VkFormat m_VkSwapChainImageFormat;
        VkExtent2D m_VkSwapChainExtent2D;
        VkRenderPass m_VkRenderPass;

        std::vector<VkImage> m_VkSwapChainImages;
        std::vector<VkImageView> m_VkSwapChainImageViews;
        std::vector<VkFramebuffer> m_VkSwapChainFramebuffers;

        std::unique_ptr<VulkanImage> m_DepthImage;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_InFlightFences;
        std::vector<VkFence> m_ImagesInFlight;

        uint32_t m_ImageIndex, m_CurrentFrameIndex;

        VkSurfaceKHR m_VkSurface;

    public:
        constexpr static uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    };
}
