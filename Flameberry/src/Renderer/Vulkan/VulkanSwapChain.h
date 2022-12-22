// #pragma once

// #include <vulkan/vulkan.h>
// #include <vector>
// #include "Core/optional.h"

// namespace Flameberry {
//     struct QueueFamilyIndices
//     {
//         flame::optional<uint32_t> GraphicsSupportedQueueFamilyIndex;
//         flame::optional<uint32_t> PresentationSupportedQueueFamilyIndex;
//     };
//     struct SwapChainDetails
//     {
//         VkSurfaceCapabilitiesKHR        SurfaceCapabilities;
//         std::vector<VkSurfaceFormatKHR> SurfaceFormats;
//         std::vector<VkPresentModeKHR>   PresentationModes;
//     };

//     class VulkanSwapChain
//     {
//     public:
//         VulkanSwapChain(VkPhysicalDevice& physicalDevice);
//         ~VulkanSwapChain();

//         SwapChainDetails GetSwapChainDetails();
//     private:
//         VkSwapchainKHR             m_VkSwapChain;
//         std::vector<VkImage>       m_VkSwapChainImages;
//         VkFormat                   m_VkSwapChainImageFormat;
//         VkExtent2D                 m_VkSwapChainExtent2D;
//         std::vector<VkImageView>   m_VkSwapChainImageViews;
//         std::vector<VkFramebuffer> m_VkSwapChainFramebuffers;

//         uint32_t m_MinImageCount;

//         VkPhysicalDevice& m_VkPhysicalDevice;
//         VkDevice& m_VkDevice;
//     };
// }