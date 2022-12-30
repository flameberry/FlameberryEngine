#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include "VulkanImage.h"

namespace Flameberry {
    class VulkanSwapChain
    {
    public:
        VulkanSwapChain(VkSurfaceKHR surface, const std::shared_ptr<VulkanSwapChain>& oldSwapChain = nullptr);
        ~VulkanSwapChain();

        VkRenderPass GetRenderPass() const { return m_VkRenderPass; }
        VkExtent2D GetExtent2D() const { return m_VkSwapChainExtent2D; }
        uint32_t GetAcquiredImageIndex() const { return m_ImageIndex; }
        VkFramebuffer GetFramebuffer(uint32_t index) { return m_VkSwapChainFramebuffers[index]; }
        uint32_t GetSwapChainImageCount() const { return (uint32_t)m_VkSwapChainImages.size(); }
        VkSwapchainKHR GetVulkanSwapChain() const { return m_VkSwapChain; }

        VkResult AcquireNextImage();
        VkResult SubmitCommandBuffer(VkCommandBuffer commandBuffer);
        void Invalidate(VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE);
    private:
        VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
        VkPresentModeKHR SelectSwapPresentationMode(const std::vector<VkPresentModeKHR>& available_presentation_modes);
        VkExtent2D SelectSwapExtent(const VkSurfaceCapabilitiesKHR& surface_capabilities);
        VkFormat GetDepthFormat();
        void CreateSwapChain(const std::shared_ptr<VulkanSwapChain>& oldSwapChain);
        void CreateRenderPass();
        void CreateFramebuffers();
        void CreateSyncObjects();
    private:
        VkSwapchainKHR m_VkSwapChain;
        VkSurfaceKHR m_VkSurface;
        VkRenderPass m_VkRenderPass;
        VkFormat m_VkSwapChainImageFormat;
        VkExtent2D m_VkSwapChainExtent2D;

        std::vector<VkImage> m_VkSwapChainImages;
        std::vector<VkImageView> m_VkSwapChainImageViews;
        std::vector<VkFramebuffer> m_VkSwapChainFramebuffers;

        std::shared_ptr<VulkanImage> m_DepthImage;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_InFlightFences;
        std::vector<VkFence> m_ImagesInFlight;

        uint32_t m_ImageIndex, m_CurrentFrameIndex;
    public:
        constexpr static uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    };
}
