#pragma once

#include <vector>

#include "Core/optional.h"
#include "VulkanVertex.h"

namespace Flameberry {
    struct QueueFamilyIndices;

    struct SwapChainDetails
    {
        VkSurfaceCapabilitiesKHR        SurfaceCapabilities;
        std::vector<VkSurfaceFormatKHR> SurfaceFormats;
        std::vector<VkPresentModeKHR>   PresentationModes;
    };

    class VulkanRenderCommand
    {
    public:
        static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);
        static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& compiledShaderCode);
        static uint32_t GetValidMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags vk_memory_property_flags);
        static bool HasStencilComponent(VkFormat format);
        static VkFormat GetSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidateFormats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
        static bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers);
        static QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
        static SwapChainDetails GetSwapChainDetails(VkPhysicalDevice vk_device, VkSurfaceKHR surface);
        static std::vector<char> LoadCompiledShaderCode(const std::string& filePath);
        static std::tuple<std::vector<VulkanVertex>, std::vector<uint32_t>> LoadModel(const std::string& filePath);
    };
}
