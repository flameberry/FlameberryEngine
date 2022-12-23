#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Flameberry {
    class VulkanDevice
    {
    public:
        VulkanDevice(VkPhysicalDevice& physicalDevice, std::vector<VkDeviceQueueCreateInfo>& deviceQueueCreateInfos, VkPhysicalDeviceFeatures physicalDeviceFeatures);
        ~VulkanDevice();

        VkDevice GetDevice() const { return m_VkDevice; }
    private:
        VkDevice m_VkDevice;

#ifdef __APPLE__
        const std::vector<const char*> m_VkDeviceExtensions = { "VK_KHR_portability_subset", VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#else
        const std::vector<const char*> m_VkDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#endif
        std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

        VkPhysicalDevice& m_VkPhysicalDevice;
    };
}
