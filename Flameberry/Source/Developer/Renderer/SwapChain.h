#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

#include "Image.h"

namespace Flameberry {

	class SwapChain
	{
	public:
		SwapChain(VkSurfaceKHR surface, const Ref<SwapChain>& oldSwapChain = nullptr);
		~SwapChain();

		VkExtent2D GetExtent2D() const { return m_VkSwapChainExtent2D; }
		uint32_t GetAcquiredImageIndex() const { return m_ImageIndex; }
		uint32_t GetSwapChainImageCount() const { return (uint32_t)m_VkSwapChainImages.size(); }
		VkFormat GetSwapChainImageFormat() const { return m_VkSwapChainImageFormat; }
		VkSwapchainKHR GetVulkanSwapChain() const { return m_VkSwapChain; }
		std::vector<VkImageView> GetImageViews() const { return m_VkSwapChainImageViews; }
		std::vector<VkImage> GetImages() const { return m_VkSwapChainImages; }

		VkResult AcquireNextImage();
		VkResult SubmitCommandBuffer(VkCommandBuffer commandBuffer);
		void Invalidate();

		static VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
		static VkPresentModeKHR SelectSwapPresentationMode(const std::vector<VkPresentModeKHR>& available_presentation_modes);
		static VkExtent2D SelectSwapExtent(const VkSurfaceCapabilitiesKHR& surface_capabilities);
		static VkFormat GetDepthFormat();

	private:
		void CreateSwapChain(const Ref<SwapChain>& oldSwapChain);
		void CreateSyncObjects();

	private:
		VkSwapchainKHR m_VkSwapChain;
		VkSurfaceKHR m_VkSurface;
		VkFormat m_VkSwapChainImageFormat;
		VkExtent2D m_VkSwapChainExtent2D;

		std::vector<VkImage> m_VkSwapChainImages;
		std::vector<VkImageView> m_VkSwapChainImageViews;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		std::vector<VkFence> m_ImagesInFlight;

		uint32_t m_ImageIndex = 0, m_CurrentFrameIndex = 0;
		uint32_t m_ImageCount = 0;

	public:
		constexpr static uint32_t MAX_FRAMES_IN_FLIGHT = 3;
	};

} // namespace Flameberry
