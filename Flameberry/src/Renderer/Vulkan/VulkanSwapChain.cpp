// #include "VulkanSwapChain.h"

// #include "Core/Core.h"
// #include "VulkanRenderer.h"

// namespace Flameberry {
//     VulkanSwapChain::VulkanSwapChain(VkPhysicalDevice& physicalDevice)
//         : m_VkPhysicalDevice(physicalDevice)
//     {
//         SwapChainDetails vk_swap_chain_details = GetSwapChainDetails();
//         VkSurfaceFormatKHR vk_surface_format = SelectSwapSurfaceFormat(vk_swap_chain_details.SurfaceFormats);
//         VkPresentModeKHR vk_presentation_mode = SelectSwapPresentationMode(vk_swap_chain_details.PresentationModes);
//         VkExtent2D vk_extent_2d = SelectSwapExtent(vk_swap_chain_details.SurfaceCapabilities);

//         m_VkSwapChainImageFormat = vk_surface_format.format;
//         m_VkSwapChainExtent2D = vk_extent_2d;

//         uint32_t imageCount = vk_swap_chain_details.SurfaceCapabilities.minImageCount + 1;

//         if ((vk_swap_chain_details.SurfaceCapabilities.maxImageCount > 0) && (imageCount > vk_swap_chain_details.SurfaceCapabilities.maxImageCount))
//             imageCount = vk_swap_chain_details.SurfaceCapabilities.maxImageCount;

//         VkSwapchainCreateInfoKHR vk_swap_chain_create_info{};
//         vk_swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
//         vk_swap_chain_create_info.surface = VulkanRenderer::GetSurface();
//         vk_swap_chain_create_info.minImageCount = imageCount;
//         vk_swap_chain_create_info.imageFormat = vk_surface_format.format;
//         vk_swap_chain_create_info.imageColorSpace = vk_surface_format.colorSpace;
//         vk_swap_chain_create_info.imageExtent = vk_extent_2d;
//         vk_swap_chain_create_info.imageArrayLayers = 1;
//         vk_swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

//         m_MinImageCount = imageCount;

//         uint32_t vk_queue_indices[2] = { m_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex , m_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex };

//         if (m_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex != m_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex)
//         {
//             vk_swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
//             vk_swap_chain_create_info.queueFamilyIndexCount = 2;
//             vk_swap_chain_create_info.pQueueFamilyIndices = vk_queue_indices;
//         }
//         else
//         {
//             vk_swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
//             vk_swap_chain_create_info.queueFamilyIndexCount = 0;
//             vk_swap_chain_create_info.pQueueFamilyIndices = nullptr;
//         }

//         vk_swap_chain_create_info.preTransform = vk_swap_chain_details.SurfaceCapabilities.currentTransform;
//         vk_swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
//         vk_swap_chain_create_info.presentMode = vk_presentation_mode;
//         vk_swap_chain_create_info.clipped = VK_TRUE;
//         vk_swap_chain_create_info.oldSwapchain = VK_NULL_HANDLE;

//         FL_ASSERT(vkCreateSwapchainKHR(m_VkDevice, &vk_swap_chain_create_info, nullptr, &m_VkSwapChain) == VK_SUCCESS, "Failed to create Vulkan Swap Chain!");
//         FL_INFO("Created Vulkan Swap Chain!");

//         uint32_t vk_swap_chain_image_count = 0;
//         vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapChain, &vk_swap_chain_image_count, nullptr);
//         m_VkSwapChainImages.resize(vk_swap_chain_image_count);
//         vkGetSwapchainImagesKHR(m_VkDevice, m_VkSwapChain, &vk_swap_chain_image_count, m_VkSwapChainImages.data());
//     }

//     VulkanSwapChain::~VulkanSwapChain()
//     {
//     }

//     SwapChainDetails VulkanSwapChain::GetSwapChainDetails()
//     {
//         SwapChainDetails vk_swap_chain_details;
//         vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_VkPhysicalDevice, VulkanRenderer::GetSurface(), &vk_swap_chain_details.SurfaceCapabilities);

//         uint32_t formatCount = 0;
//         vkGetPhysicalDeviceSurfaceFormatsKHR(m_VkPhysicalDevice, VulkanRenderer::GetSurface(), &formatCount, nullptr);

//         if (formatCount)
//         {
//             vk_swap_chain_details.SurfaceFormats.resize(formatCount);
//             vkGetPhysicalDeviceSurfaceFormatsKHR(m_VkPhysicalDevice, VulkanRenderer::GetSurface(), &formatCount, vk_swap_chain_details.SurfaceFormats.data());
//         }

//         uint32_t presentationModeCount = 0;
//         vkGetPhysicalDeviceSurfacePresentModesKHR(m_VkPhysicalDevice, VulkanRenderer::GetSurface(), &presentationModeCount, nullptr);

//         if (presentationModeCount)
//         {
//             vk_swap_chain_details.PresentationModes.resize(presentationModeCount);
//             vkGetPhysicalDeviceSurfacePresentModesKHR(m_VkPhysicalDevice, VulkanRenderer::GetSurface(), &presentationModeCount, vk_swap_chain_details.PresentationModes.data());
//         }

//         return vk_swap_chain_details;
//     }
// }