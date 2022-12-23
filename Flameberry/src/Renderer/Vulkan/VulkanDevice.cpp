#include "VulkanDevice.h"

#include "Core/Core.h"

namespace Flameberry {
    VulkanDevice::VulkanDevice(VkPhysicalDevice& physicalDevice, std::vector<VkDeviceQueueCreateInfo>& deviceQueueCreateInfos, VkPhysicalDeviceFeatures physicalDeviceFeatures)
        : m_VkPhysicalDevice(physicalDevice)
    {
        // Creating Vulkan Logical Device
        VkDeviceCreateInfo vk_device_create_info{};
        vk_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        vk_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());
        vk_device_create_info.pQueueCreateInfos = deviceQueueCreateInfos.data();

        vk_device_create_info.pEnabledFeatures = &physicalDeviceFeatures;

        vk_device_create_info.enabledExtensionCount = static_cast<uint32_t>(m_VkDeviceExtensions.size());
        vk_device_create_info.ppEnabledExtensionNames = m_VkDeviceExtensions.data();

#ifdef FL_DEBUG
        vk_device_create_info.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        vk_device_create_info.ppEnabledLayerNames = m_ValidationLayers.data();
#else
        vk_device_create_info.enabledLayerCount = 0;
#endif

        FL_ASSERT(vkCreateDevice(m_VkPhysicalDevice, &vk_device_create_info, nullptr, &m_VkDevice) == VK_SUCCESS, "Failed to create Vulkan Logical Device!");
    }

    VulkanDevice::~VulkanDevice()
    {
    }
}