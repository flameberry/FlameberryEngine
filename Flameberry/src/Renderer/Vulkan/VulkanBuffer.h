#pragma once

#include <vulkan/vulkan.h>

namespace Flameberry {
    class VulkanBuffer
    {
    public:
        VulkanBuffer(VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags);
        ~VulkanBuffer();

        const VkBuffer& GetBuffer() const { return m_VkBuffer; }
        VkResult MapMemory(VkDeviceSize size, VkDeviceSize offset = 0);
        void WriteToBuffer(const void* data, VkDeviceSize size, VkDeviceSize offset);
        void UnmapMemory();

        void DestroyBuffer();
    private:
        VkBuffer m_VkBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_VkBufferDeviceMemory = VK_NULL_HANDLE;
        void* m_VkBufferMappedMemory = nullptr;
    };
}