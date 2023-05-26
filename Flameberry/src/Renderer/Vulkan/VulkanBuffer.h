#pragma once

#include <vulkan/vulkan.h>

namespace Flameberry {
    class VulkanBuffer
    {
    public:
        VulkanBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags);
        VulkanBuffer(VkDeviceSize instanceSize, uint32_t instanceCount, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment);
        ~VulkanBuffer();

        const VkBuffer& GetBuffer() const { return m_VkBuffer; }
        VkResult MapMemory(VkDeviceSize size, VkDeviceSize offset = 0);
        void WriteToBuffer(const void* data, VkDeviceSize size, VkDeviceSize offset);
        void WriteToIndex(const void* data, uint32_t index);
        VkResult Flush(VkDeviceSize size, VkDeviceSize offset);
        VkResult FlushIndex(int index);
        void UnmapMemory();

        void DestroyBuffer();

        const void* GetMappedMemory() const { return m_VkBufferMappedMemory; };
    private:
        void CreateBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize minOffsetAlignment);
        VkDeviceSize GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);
    private:
        VkBuffer m_VkBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_VkBufferDeviceMemory = VK_NULL_HANDLE;
        void* m_VkBufferMappedMemory = nullptr;

        VkDeviceSize m_InstanceSize = 0, m_AlignmentSize = 1;
    };
}