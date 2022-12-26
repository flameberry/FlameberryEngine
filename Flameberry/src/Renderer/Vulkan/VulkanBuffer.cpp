#include "VulkanBuffer.h"

#include "Core/Core.h"
#include "VulkanRenderCommand.h"
#include "VulkanContext.h"

namespace Flameberry {
    VulkanBuffer::VulkanBuffer(VkDeviceSize deviceSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        const auto& physicalDevice = VulkanContext::GetPhysicalDevice();

        VkBufferCreateInfo vk_buffer_create_info{};
        vk_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vk_buffer_create_info.size = deviceSize;
        vk_buffer_create_info.usage = bufferUsageFlags;
        vk_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        FL_ASSERT(vkCreateBuffer(device, &vk_buffer_create_info, nullptr, &m_VkBuffer) == VK_SUCCESS, "Failed to create Buffer!");

        VkMemoryRequirements vk_memory_requirements{};
        vkGetBufferMemoryRequirements(device, m_VkBuffer, &vk_memory_requirements);

        VkMemoryAllocateInfo vk_memory_allocate_info{};
        vk_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vk_memory_allocate_info.allocationSize = vk_memory_requirements.size;
        vk_memory_allocate_info.memoryTypeIndex = VulkanRenderCommand::GetValidMemoryTypeIndex(physicalDevice, vk_memory_requirements.memoryTypeBits, memoryPropertyFlags);

        FL_ASSERT(vkAllocateMemory(device, &vk_memory_allocate_info, nullptr, &m_VkBufferDeviceMemory) == VK_SUCCESS, "Failed to allocate memory!");
        vkBindBufferMemory(device, m_VkBuffer, m_VkBufferDeviceMemory, 0);
    }

    VulkanBuffer::~VulkanBuffer()
    {
        DestroyBuffer();
    }

    VkResult VulkanBuffer::MapMemory(VkDeviceSize size, VkDeviceSize offset)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        FL_ASSERT(size && m_VkBufferDeviceMemory, "Cannot Map memory of size: 0!");
        return vkMapMemory(device, m_VkBufferDeviceMemory, offset, size, 0, &m_VkBufferMappedMemory);
    }

    void VulkanBuffer::UnmapMemory()
    {
        if (m_VkBufferMappedMemory)
        {
            const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
            vkUnmapMemory(device, m_VkBufferDeviceMemory);
            m_VkBufferMappedMemory = nullptr;
        }
    }

    void VulkanBuffer::WriteToBuffer(const void* data, VkDeviceSize size, VkDeviceSize offset)
    {
        if (size == VK_WHOLE_SIZE)
            memcpy(m_VkBufferMappedMemory, data, size);
        else
        {
            char* memoryOffset = (char*)m_VkBufferMappedMemory;
            memoryOffset += offset;
            memcpy(memoryOffset, data, size);
        }
    }

    void VulkanBuffer::DestroyBuffer()
    {
        if (m_VkBuffer != VK_NULL_HANDLE && m_VkBufferDeviceMemory != VK_NULL_HANDLE)
        {
            const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
            vkDestroyBuffer(device, m_VkBuffer, nullptr);
            vkFreeMemory(device, m_VkBufferDeviceMemory, nullptr);

            m_VkBuffer = VK_NULL_HANDLE;
            m_VkBufferDeviceMemory = VK_NULL_HANDLE;
        }
    }
}