#pragma once

#include <vulkan/vulkan.h>

namespace Flameberry {
    class VulkanBuffer
    {
    public:
        VulkanBuffer(VkDevice& deviceInstance, VkDeviceSize deviceSize, const void* data);
        ~VulkanBuffer();
    private:
        VkBuffer m_VkBuffer;
        VkDeviceMemory m_VkBufferDeviceMemory;

        VkDevice& m_VkDevice;
    };
}