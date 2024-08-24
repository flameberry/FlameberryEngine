#include "SwapChain.h"

#include "VulkanDebug.h"

#include "RenderCommand.h"
#include "VulkanContext.h"
#include "Image.h"

namespace Flameberry {

	SwapChain::SwapChain(VkSurfaceKHR surface, const Ref<SwapChain>& oldSwapChain)
		: m_VkSurface(surface)
	{
		CreateSwapChain(oldSwapChain);
		CreateSyncObjects();
	}

	void SwapChain::CreateSwapChain(const Ref<SwapChain>& oldSwapChain)
	{
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		const auto& queueFamilyIndices = VulkanContext::GetCurrentDevice()->GetQueueFamilyIndices();
		const auto& physicalDevice = VulkanContext::GetPhysicalDevice();

		SwapChainDetails vk_swap_chain_details = RenderCommand::GetSwapChainDetails(physicalDevice, m_VkSurface);
		VkSurfaceFormatKHR vk_surface_format = SelectSwapSurfaceFormat(vk_swap_chain_details.SurfaceFormats);
		VkPresentModeKHR vk_presentation_mode = SelectSwapPresentationMode(vk_swap_chain_details.PresentationModes);
		VkExtent2D vk_extent_2d = SelectSwapExtent(vk_swap_chain_details.SurfaceCapabilities);

		m_VkSwapChainImageFormat = vk_surface_format.format;
		m_VkSwapChainExtent2D = vk_extent_2d;

		uint32_t imageCount = vk_swap_chain_details.SurfaceCapabilities.minImageCount + 1;

		if ((vk_swap_chain_details.SurfaceCapabilities.maxImageCount > 0) && (imageCount > vk_swap_chain_details.SurfaceCapabilities.maxImageCount))
			imageCount = vk_swap_chain_details.SurfaceCapabilities.maxImageCount;

		VkSwapchainCreateInfoKHR vulkanSwapchainCreateInfo{};
		vulkanSwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		vulkanSwapchainCreateInfo.surface = m_VkSurface;
		vulkanSwapchainCreateInfo.minImageCount = imageCount;
		vulkanSwapchainCreateInfo.imageFormat = vk_surface_format.format;
		vulkanSwapchainCreateInfo.imageColorSpace = vk_surface_format.colorSpace;
		vulkanSwapchainCreateInfo.imageExtent = vk_extent_2d;
		vulkanSwapchainCreateInfo.imageArrayLayers = 1;
		vulkanSwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t vk_queue_indices[] = {
			queueFamilyIndices.GraphicsQueueFamilyIndex,
			queueFamilyIndices.PresentationSupportedQueueFamilyIndex
		};

		if (queueFamilyIndices.GraphicsQueueFamilyIndex != queueFamilyIndices.PresentationSupportedQueueFamilyIndex)
		{
			vulkanSwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			vulkanSwapchainCreateInfo.queueFamilyIndexCount = sizeof(vk_queue_indices) / sizeof(uint32_t);
			vulkanSwapchainCreateInfo.pQueueFamilyIndices = vk_queue_indices;
		}
		else
		{
			vulkanSwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			vulkanSwapchainCreateInfo.queueFamilyIndexCount = 0;
			vulkanSwapchainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		vulkanSwapchainCreateInfo.preTransform = vk_swap_chain_details.SurfaceCapabilities.currentTransform;
		vulkanSwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		vulkanSwapchainCreateInfo.presentMode = vk_presentation_mode;
		vulkanSwapchainCreateInfo.clipped = VK_TRUE;
		vulkanSwapchainCreateInfo.oldSwapchain = oldSwapChain ? oldSwapChain->m_VkSwapChain : VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &vulkanSwapchainCreateInfo, nullptr, &m_VkSwapChain));
		FBY_INFO("Created Vulkan Swap Chain!");

		vkGetSwapchainImagesKHR(device, m_VkSwapChain, &m_ImageCount, nullptr);
		m_VkSwapChainImages.resize(m_ImageCount);
		vkGetSwapchainImagesKHR(device, m_VkSwapChain, &m_ImageCount, m_VkSwapChainImages.data());
	}

	VkResult SwapChain::AcquireNextImage()
	{
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vkWaitForFences(device, 1, &m_InFlightFences[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);

		VkResult imageAcquireStatus = vkAcquireNextImageKHR(device, m_VkSwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrameIndex], VK_NULL_HANDLE, &m_ImageIndex);
		return imageAcquireStatus;
	}

	VkResult SwapChain::SubmitCommandBuffer(VkCommandBuffer commandBuffer)
	{
		const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		const auto graphicsQueue = VulkanContext::GetCurrentDevice()->GetGraphicsQueue();
		const auto presentationQueue = VulkanContext::GetCurrentDevice()->GetPresentationQueue();

		if (m_ImagesInFlight[m_ImageIndex] != VK_NULL_HANDLE) // Check if a previous frame is using this image (i.e. there is its fence to wait on)
			vkWaitForFences(device, 1, &m_ImagesInFlight[m_ImageIndex], VK_TRUE, UINT64_MAX);
		m_ImagesInFlight[m_ImageIndex] = m_InFlightFences[m_CurrentFrameIndex]; // Mark the image as now being in use by this frame

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrameIndex] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrameIndex] };

		// Submit Queue
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(device, 1, &m_InFlightFences[m_CurrentFrameIndex]);

		VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrameIndex]));

		VkSwapchainKHR swapchains[] = { m_VkSwapChain };

		VkPresentInfoKHR vk_present_info{};
		vk_present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		vk_present_info.waitSemaphoreCount = 1;
		vk_present_info.pWaitSemaphores = signalSemaphores;
		vk_present_info.pResults = nullptr;

		vk_present_info.swapchainCount = 1;
		vk_present_info.pSwapchains = swapchains;
		vk_present_info.pImageIndices = &m_ImageIndex;

		VkResult queuePresentStatus = vkQueuePresentKHR(presentationQueue, &vk_present_info);
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
		return queuePresentStatus;
	}

	VkFormat SwapChain::GetDepthFormat()
	{
		const auto& physicalDevice = VulkanContext::GetPhysicalDevice();
		return RenderCommand::GetSupportedFormat(
			physicalDevice,
			{ VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	void SwapChain::CreateSyncObjects()
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
		FBY_INFO("Created {} Image Available Semaphores, Render Finished Semaphores, and 'in flight fences'!", MAX_FRAMES_IN_FLIGHT);
	}

	SwapChain::~SwapChain()
	{
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vkDeviceWaitIdle(device);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, m_InFlightFences[i], nullptr);
		}
		FBY_INFO("Destroyed {} Image Available Semaphores, Render Finished Semaphores and 'in flight fences'!", MAX_FRAMES_IN_FLIGHT);

		vkDestroySwapchainKHR(device, m_VkSwapChain, nullptr);
	}

	void SwapChain::Invalidate()
	{
		FBY_INFO("Invalidating SwapChain...");

		const auto device = VulkanContext::GetCurrentDevice();
		device->WaitIdle();
		device->WaitIdleGraphicsQueue();
		device->WaitIdleComputeQueue(); // TODO: Is this needed?

		vkDestroySwapchainKHR(device->GetVulkanDevice(), m_VkSwapChain, nullptr);

		CreateSwapChain(nullptr);
	}

	VkSurfaceFormatKHR SwapChain::SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
	{
		for (const auto& format : available_formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return format;
		}
		// Implement choosing of the next best format after UNORM R8B8G8A8 format
		return available_formats[0];
	}

	VkPresentModeKHR SwapChain::SelectSwapPresentationMode(const std::vector<VkPresentModeKHR>& available_presentation_modes)
	{
		for (const auto& mode : available_presentation_modes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D SwapChain::SelectSwapExtent(const VkSurfaceCapabilitiesKHR& surface_capabilities)
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

} // namespace Flameberry
