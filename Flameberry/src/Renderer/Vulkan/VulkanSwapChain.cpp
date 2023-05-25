#include "VulkanSwapChain.h"

#include "VulkanDebug.h"

#include "VulkanRenderCommand.h"
#include "VulkanContext.h"
#include "VulkanImage.h"

namespace Flameberry {
    VulkanSwapChain::VulkanSwapChain(VkSurfaceKHR surface, const std::shared_ptr<VulkanSwapChain>& oldSwapChain)
        : m_VkSurface(surface)
    {
        CreateSwapChain(oldSwapChain);
    }

    void VulkanSwapChain::CreateSwapChain(const std::shared_ptr<VulkanSwapChain>& oldSwapChain)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        const auto& queueFamilyIndices = VulkanContext::GetCurrentDevice()->GetQueueFamilyIndices();
        const auto& physicalDevice = VulkanContext::GetPhysicalDevice();

        SwapChainDetails vk_swap_chain_details = VulkanRenderCommand::GetSwapChainDetails(physicalDevice, m_VkSurface);
        VkSurfaceFormatKHR vk_surface_format = SelectSwapSurfaceFormat(vk_swap_chain_details.SurfaceFormats);
        VkPresentModeKHR vk_presentation_mode = SelectSwapPresentationMode(vk_swap_chain_details.PresentationModes);
        VkExtent2D vk_extent_2d = SelectSwapExtent(vk_swap_chain_details.SurfaceCapabilities);

        m_VkSwapChainImageFormat = vk_surface_format.format;
        m_VkSwapChainExtent2D = vk_extent_2d;

        uint32_t imageCount = vk_swap_chain_details.SurfaceCapabilities.minImageCount + 1;

        if ((vk_swap_chain_details.SurfaceCapabilities.maxImageCount > 0) && (imageCount > vk_swap_chain_details.SurfaceCapabilities.maxImageCount))
            imageCount = vk_swap_chain_details.SurfaceCapabilities.maxImageCount;

        VkSwapchainCreateInfoKHR vk_swap_chain_create_info{};
        vk_swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        vk_swap_chain_create_info.surface = m_VkSurface;
        vk_swap_chain_create_info.minImageCount = imageCount;
        vk_swap_chain_create_info.imageFormat = vk_surface_format.format;
        vk_swap_chain_create_info.imageColorSpace = vk_surface_format.colorSpace;
        vk_swap_chain_create_info.imageExtent = vk_extent_2d;
        vk_swap_chain_create_info.imageArrayLayers = 1;
        vk_swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t vk_queue_indices[2] = { queueFamilyIndices.GraphicsSupportedQueueFamilyIndex , queueFamilyIndices.PresentationSupportedQueueFamilyIndex };

        if (queueFamilyIndices.GraphicsSupportedQueueFamilyIndex != queueFamilyIndices.PresentationSupportedQueueFamilyIndex)
        {
            vk_swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            vk_swap_chain_create_info.queueFamilyIndexCount = 2;
            vk_swap_chain_create_info.pQueueFamilyIndices = vk_queue_indices;
        }
        else
        {
            vk_swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vk_swap_chain_create_info.queueFamilyIndexCount = 0;
            vk_swap_chain_create_info.pQueueFamilyIndices = nullptr;
        }

        vk_swap_chain_create_info.preTransform = vk_swap_chain_details.SurfaceCapabilities.currentTransform;
        vk_swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        vk_swap_chain_create_info.presentMode = vk_presentation_mode;
        vk_swap_chain_create_info.clipped = VK_TRUE;
        vk_swap_chain_create_info.oldSwapchain = oldSwapChain ? oldSwapChain->m_VkSwapChain : VK_NULL_HANDLE;

        VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &vk_swap_chain_create_info, nullptr, &m_VkSwapChain));
        FL_INFO("Created Vulkan Swap Chain!");

        vkGetSwapchainImagesKHR(device, m_VkSwapChain, &m_ImageCount, nullptr);
        m_VkSwapChainImages.resize(m_ImageCount);
        vkGetSwapchainImagesKHR(device, m_VkSwapChain, &m_ImageCount, m_VkSwapChainImages.data());

        CreateSyncObjects();
    }

    VkResult VulkanSwapChain::AcquireNextImage()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkWaitForFences(device, 1, &m_InFlightFences[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);

        VkResult imageAcquireStatus = vkAcquireNextImageKHR(device, m_VkSwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrameIndex], VK_NULL_HANDLE, &m_ImageIndex);
        return imageAcquireStatus;
    }

    VkResult VulkanSwapChain::SubmitCommandBuffer(VkCommandBuffer commandBuffer)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        const auto& graphicsQueue = VulkanContext::GetCurrentDevice()->GetGraphicsQueue();
        const auto& presentationQueue = VulkanContext::GetCurrentDevice()->GetPresentationQueue();

        if (m_ImagesInFlight[m_ImageIndex] != VK_NULL_HANDLE) // Check if a previous frame is using this image (i.e. there is its fence to wait on)
            vkWaitForFences(device, 1, &m_ImagesInFlight[m_ImageIndex], VK_TRUE, UINT64_MAX);
        m_ImagesInFlight[m_ImageIndex] = m_InFlightFences[m_CurrentFrameIndex]; // Mark the image as now being in use by this frame

        // Submit Queue
        VkSubmitInfo vk_submit_info{};
        vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_semaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrameIndex] };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        vk_submit_info.waitSemaphoreCount = 1;
        vk_submit_info.pWaitSemaphores = wait_semaphores;
        vk_submit_info.pWaitDstStageMask = wait_stages;
        vk_submit_info.commandBufferCount = 1;
        vk_submit_info.pCommandBuffers = &commandBuffer;

        VkSemaphore signal_semaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrameIndex] };
        vk_submit_info.signalSemaphoreCount = 1;
        vk_submit_info.pSignalSemaphores = signal_semaphores;

        vkResetFences(device, 1, &m_InFlightFences[m_CurrentFrameIndex]);
        VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &vk_submit_info, m_InFlightFences[m_CurrentFrameIndex]));

        VkPresentInfoKHR vk_present_info{};
        vk_present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        vk_present_info.waitSemaphoreCount = 1;
        vk_present_info.pWaitSemaphores = signal_semaphores;

        VkSwapchainKHR vk_swap_chains[] = { m_VkSwapChain };
        vk_present_info.swapchainCount = 1;
        vk_present_info.pSwapchains = vk_swap_chains;
        vk_present_info.pImageIndices = &m_ImageIndex;
        vk_present_info.pResults = nullptr;

        VkResult queuePresentStatus = vkQueuePresentKHR(presentationQueue, &vk_present_info);
        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
        return queuePresentStatus;
    }

    VkFormat VulkanSwapChain::GetDepthFormat()
    {
        const auto& physicalDevice = VulkanContext::GetPhysicalDevice();
        return VulkanRenderCommand::GetSupportedFormat(
            physicalDevice,
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    void VulkanSwapChain::CreateSyncObjects()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_ImagesInFlight.resize(m_VkSwapChainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo vk_semaphore_create_info{};
        vk_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo vk_fence_create_info{};
        vk_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vk_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VK_CHECK_RESULT(vkCreateSemaphore(device, &vk_semaphore_create_info, nullptr, &m_ImageAvailableSemaphores[i]));
            VK_CHECK_RESULT(vkCreateSemaphore(device, &vk_semaphore_create_info, nullptr, &m_RenderFinishedSemaphores[i]));
            VK_CHECK_RESULT(vkCreateFence(device, &vk_fence_create_info, nullptr, &m_InFlightFences[i]));
        }
        FL_INFO("Created {0} Image Available Semaphores, Render Finished Semaphores, and 'in flight fences'!", MAX_FRAMES_IN_FLIGHT);
    }

    VulkanSwapChain::~VulkanSwapChain()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDeviceWaitIdle(device);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, m_InFlightFences[i], nullptr);
        }
        FL_INFO("Destroyed {0} Image Available Semaphores, Render Finished Semaphores and 'in flight fences'!", MAX_FRAMES_IN_FLIGHT);

        vkDestroySwapchainKHR(device, m_VkSwapChain, nullptr);
    }

    void VulkanSwapChain::Invalidate(VkSwapchainKHR oldSwapChain)
    {
        FL_INFO("Invalidating SwapChain...");
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(VulkanContext::GetCurrentWindow()->GetGLFWwindow(), &width, &height);
            glfwWaitEvents();
        }

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        const auto& physicalDevice = VulkanContext::GetPhysicalDevice();
        const auto& queueFamilyIndices = VulkanContext::GetCurrentDevice()->GetQueueFamilyIndices();

        vkDeviceWaitIdle(device);
        FL_INFO("Device is idle now...");

        SwapChainDetails vk_swap_chain_details = VulkanRenderCommand::GetSwapChainDetails(physicalDevice, m_VkSurface);
        VkSurfaceFormatKHR vk_surface_format = SelectSwapSurfaceFormat(vk_swap_chain_details.SurfaceFormats);
        VkPresentModeKHR vk_presentation_mode = SelectSwapPresentationMode(vk_swap_chain_details.PresentationModes);
        VkExtent2D vk_extent_2d = SelectSwapExtent(vk_swap_chain_details.SurfaceCapabilities);

        m_VkSwapChainImageFormat = vk_surface_format.format;
        m_VkSwapChainExtent2D = vk_extent_2d;

        uint32_t imageCount = vk_swap_chain_details.SurfaceCapabilities.minImageCount + 1;

        if ((vk_swap_chain_details.SurfaceCapabilities.maxImageCount > 0) && (imageCount > vk_swap_chain_details.SurfaceCapabilities.maxImageCount))
            imageCount = vk_swap_chain_details.SurfaceCapabilities.maxImageCount;

        VkSwapchainCreateInfoKHR vk_swap_chain_create_info{};
        vk_swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        vk_swap_chain_create_info.surface = m_VkSurface;
        vk_swap_chain_create_info.minImageCount = imageCount;
        vk_swap_chain_create_info.imageFormat = vk_surface_format.format;
        vk_swap_chain_create_info.imageColorSpace = vk_surface_format.colorSpace;
        vk_swap_chain_create_info.imageExtent = vk_extent_2d;
        vk_swap_chain_create_info.imageArrayLayers = 1;
        vk_swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t vk_queue_indices[2] = { queueFamilyIndices.GraphicsSupportedQueueFamilyIndex , queueFamilyIndices.PresentationSupportedQueueFamilyIndex };

        if (queueFamilyIndices.GraphicsSupportedQueueFamilyIndex != queueFamilyIndices.PresentationSupportedQueueFamilyIndex)
        {
            vk_swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            vk_swap_chain_create_info.queueFamilyIndexCount = 2;
            vk_swap_chain_create_info.pQueueFamilyIndices = vk_queue_indices;
        }
        else
        {
            vk_swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vk_swap_chain_create_info.queueFamilyIndexCount = 0;
            vk_swap_chain_create_info.pQueueFamilyIndices = nullptr;
        }

        vk_swap_chain_create_info.preTransform = vk_swap_chain_details.SurfaceCapabilities.currentTransform;
        vk_swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        vk_swap_chain_create_info.presentMode = vk_presentation_mode;
        vk_swap_chain_create_info.clipped = VK_TRUE;
        vk_swap_chain_create_info.oldSwapchain = oldSwapChain; // TODO

        VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &vk_swap_chain_create_info, nullptr, &m_VkSwapChain));
        FL_INFO("Created Vulkan Swap Chain!");

        vkDestroySwapchainKHR(device, oldSwapChain, nullptr);

        uint32_t vk_swap_chain_image_count = 0;
        vkGetSwapchainImagesKHR(device, m_VkSwapChain, &vk_swap_chain_image_count, nullptr);
        m_VkSwapChainImages.resize(vk_swap_chain_image_count);
        vkGetSwapchainImagesKHR(device, m_VkSwapChain, &vk_swap_chain_image_count, m_VkSwapChainImages.data());
    }

    VkSurfaceFormatKHR VulkanSwapChain::SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
    {
        for (const auto& format : available_formats)
        {
            // if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        }
        // Implement choosing of the next best format after sRGB B8G8A8 format
        return available_formats[0];
    }

    VkPresentModeKHR VulkanSwapChain::SelectSwapPresentationMode(const std::vector<VkPresentModeKHR>& available_presentation_modes)
    {
        for (const auto& mode : available_presentation_modes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return mode;
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapChain::SelectSwapExtent(const VkSurfaceCapabilitiesKHR& surface_capabilities)
    {
        if (surface_capabilities.currentExtent.width != UINT32_MAX)
            return surface_capabilities.currentExtent;
        else
        {
            VulkanWindow* pWindow = VulkanContext::GetCurrentWindow();
            int width, height;
            glfwGetFramebufferSize(pWindow->GetGLFWwindow(), &width, &height);

            VkExtent2D actual_extent = {
                static_cast<uint32_t>(width), static_cast<uint32_t>(height)
            };

            actual_extent.width = std::clamp(actual_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
            actual_extent.height = std::clamp(actual_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
            return actual_extent;
        }
    }
}