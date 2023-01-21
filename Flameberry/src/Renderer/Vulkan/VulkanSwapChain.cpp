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

        uint32_t vk_swap_chain_image_count = 0;
        vkGetSwapchainImagesKHR(device, m_VkSwapChain, &vk_swap_chain_image_count, nullptr);
        m_VkSwapChainImages.resize(vk_swap_chain_image_count);
        vkGetSwapchainImagesKHR(device, m_VkSwapChain, &vk_swap_chain_image_count, m_VkSwapChainImages.data());

        // Image Views creation
        m_VkSwapChainImageViews.resize(m_VkSwapChainImages.size());

        for (uint32_t i = 0; i < m_VkSwapChainImages.size(); i++)
        {
            VkImageViewCreateInfo vk_image_view_create_info{};
            vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            vk_image_view_create_info.image = m_VkSwapChainImages[i];
            vk_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vk_image_view_create_info.format = m_VkSwapChainImageFormat;
            vk_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vk_image_view_create_info.subresourceRange.baseMipLevel = 0;
            vk_image_view_create_info.subresourceRange.levelCount = 1;
            vk_image_view_create_info.subresourceRange.baseArrayLayer = 0;
            vk_image_view_create_info.subresourceRange.layerCount = 1;

            VK_CHECK_RESULT(vkCreateImageView(device, &vk_image_view_create_info, nullptr, &m_VkSwapChainImageViews[i]));
        }

        // Create Depth Resources
        VkFormat depthFormat = GetDepthFormat();
        m_DepthImage = VulkanImage::Create(m_VkSwapChainExtent2D.width, m_VkSwapChainExtent2D.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

        CreateRenderPass();
        CreateFramebuffers();
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
        m_ImagesInFlight[m_ImageIndex] = m_InFlightFences[m_CurrentFrameIndex];// Mark the image as now being in use by this frame

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

    void VulkanSwapChain::CreateFramebuffers()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        m_VkSwapChainFramebuffers.resize(m_VkSwapChainImageViews.size());

        for (size_t i = 0; i < m_VkSwapChainImageViews.size(); i++)
        {
            std::vector<VkImageView> attachments = { m_VkSwapChainImageViews[i], m_DepthImage->GetImageView() };

            VkFramebufferCreateInfo vk_framebuffer_create_info{};
            vk_framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            vk_framebuffer_create_info.renderPass = m_VkRenderPass;
            vk_framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
            vk_framebuffer_create_info.pAttachments = attachments.data();
            vk_framebuffer_create_info.width = m_VkSwapChainExtent2D.width;
            vk_framebuffer_create_info.height = m_VkSwapChainExtent2D.height;
            vk_framebuffer_create_info.layers = 1;

            VK_CHECK_RESULT(vkCreateFramebuffer(device, &vk_framebuffer_create_info, nullptr, &m_VkSwapChainFramebuffers[i]));
        }
    }

    void VulkanSwapChain::CreateRenderPass()
    {
        VkSubpassDependency vk_subpass_dependency{};
        vk_subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        vk_subpass_dependency.dstSubpass = 0;
        vk_subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        vk_subpass_dependency.srcAccessMask = 0;
        vk_subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        vk_subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription vk_color_attachment_description{};
        vk_color_attachment_description.format = m_VkSwapChainImageFormat;
        vk_color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        vk_color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        vk_color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        vk_color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vk_color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // vk_color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        vk_color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // For ImGui

        VkAttachmentReference vk_color_attachment_reference{};
        vk_color_attachment_reference.attachment = 0;
        vk_color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription vk_depth_attachment_desc{};
        vk_depth_attachment_desc.format = GetDepthFormat();
        vk_depth_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
        vk_depth_attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        vk_depth_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_depth_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        vk_depth_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        vk_depth_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vk_depth_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference vk_depth_attachment_ref{};
        vk_depth_attachment_ref.attachment = 1;
        vk_depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription vk_subpass_description{};
        vk_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        vk_subpass_description.colorAttachmentCount = 1;
        vk_subpass_description.pColorAttachments = &vk_color_attachment_reference;
        vk_subpass_description.pDepthStencilAttachment = &vk_depth_attachment_ref;

        std::array<VkAttachmentDescription, 2> attachments = { vk_color_attachment_description, vk_depth_attachment_desc };

        VkRenderPassCreateInfo vk_render_pass_create_info{};
        vk_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        vk_render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        vk_render_pass_create_info.pAttachments = attachments.data();
        vk_render_pass_create_info.subpassCount = 1;
        vk_render_pass_create_info.pSubpasses = &vk_subpass_description;
        vk_render_pass_create_info.dependencyCount = 1;
        vk_render_pass_create_info.pDependencies = &vk_subpass_dependency;

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VK_CHECK_RESULT(vkCreateRenderPass(device, &vk_render_pass_create_info, nullptr, &m_VkRenderPass));
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

        for (auto& framebuffer : m_VkSwapChainFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);

        vkDestroyRenderPass(device, m_VkRenderPass, nullptr);

        for (auto& imageView : m_VkSwapChainImageViews)
            vkDestroyImageView(device, imageView, nullptr);

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

        for (auto& framebuffer : m_VkSwapChainFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);

        for (auto& imageView : m_VkSwapChainImageViews)
            vkDestroyImageView(device, imageView, nullptr);

        vkDestroySwapchainKHR(device, oldSwapChain, nullptr);

        uint32_t vk_swap_chain_image_count = 0;
        vkGetSwapchainImagesKHR(device, m_VkSwapChain, &vk_swap_chain_image_count, nullptr);
        m_VkSwapChainImages.resize(vk_swap_chain_image_count);
        vkGetSwapchainImagesKHR(device, m_VkSwapChain, &vk_swap_chain_image_count, m_VkSwapChainImages.data());

        // Image Views creation
        m_VkSwapChainImageViews.resize(m_VkSwapChainImages.size());

        for (uint32_t i = 0; i < m_VkSwapChainImages.size(); i++)
        {
            VkImageViewCreateInfo vk_image_view_create_info{};
            vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            vk_image_view_create_info.image = m_VkSwapChainImages[i];
            vk_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vk_image_view_create_info.format = m_VkSwapChainImageFormat;
            vk_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            vk_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vk_image_view_create_info.subresourceRange.baseMipLevel = 0;
            vk_image_view_create_info.subresourceRange.levelCount = 1;
            vk_image_view_create_info.subresourceRange.baseArrayLayer = 0;
            vk_image_view_create_info.subresourceRange.layerCount = 1;

            VK_CHECK_RESULT(vkCreateImageView(device, &vk_image_view_create_info, nullptr, &m_VkSwapChainImageViews[i]));
        }

        // Create Depth Resources
        VkFormat depthFormat = GetDepthFormat();
        m_DepthImage = VulkanImage::Create(m_VkSwapChainExtent2D.width, m_VkSwapChainExtent2D.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

        CreateFramebuffers();
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
