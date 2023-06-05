#include "Buffer.h"

#include "VulkanDebug.h"
#include "VulkanRenderCommand.h"
#include "VulkanContext.h"

namespace Flameberry {
    VkDeviceSize Buffer::GetAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
        if (minOffsetAlignment > 0)
            return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
        return instanceSize;
    }

    Buffer::Buffer(const BufferSpecification& specification)
        : m_BufferSpec(specification)
    {
        m_AlignmentSize = GetAlignment(m_BufferSpec.InstanceSize, m_BufferSpec.MinOffsetAlignment);
        VkDeviceSize bufferSize = m_AlignmentSize * m_BufferSpec.InstanceCount;

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        const auto& physicalDevice = VulkanContext::GetPhysicalDevice();

        VkBufferCreateInfo vk_buffer_create_info{};
        vk_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vk_buffer_create_info.size = bufferSize;
        vk_buffer_create_info.usage = m_BufferSpec.Usage;
        vk_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK_RESULT(vkCreateBuffer(device, &vk_buffer_create_info, nullptr, &m_VkBuffer));

        VkMemoryRequirements vk_memory_requirements{};
        vkGetBufferMemoryRequirements(device, m_VkBuffer, &vk_memory_requirements);

        VkMemoryAllocateInfo vk_memory_allocate_info{};
        vk_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vk_memory_allocate_info.allocationSize = vk_memory_requirements.size;
        vk_memory_allocate_info.memoryTypeIndex = VulkanRenderCommand::GetValidMemoryTypeIndex(physicalDevice, vk_memory_requirements.memoryTypeBits, m_BufferSpec.MemoryProperties);

        VK_CHECK_RESULT(vkAllocateMemory(device, &vk_memory_allocate_info, nullptr, &m_VkBufferDeviceMemory));
        vkBindBufferMemory(device, m_VkBuffer, m_VkBufferDeviceMemory, 0);
    }

    Buffer::~Buffer()
    {
        DestroyBuffer();
    }

    VkResult Buffer::MapMemory(VkDeviceSize size, VkDeviceSize offset)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        FL_ASSERT(size && m_VkBufferDeviceMemory, "Cannot Map memory of size: 0!");
        return vkMapMemory(device, m_VkBufferDeviceMemory, offset, size, 0, &m_VkBufferMappedMemory);
    }

    void Buffer::UnmapMemory()
    {
        if (m_VkBufferMappedMemory)
        {
            const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
            vkUnmapMemory(device, m_VkBufferDeviceMemory);
            m_VkBufferMappedMemory = nullptr;
        }
    }

    void Buffer::WriteToBuffer(const void* data, VkDeviceSize size, VkDeviceSize offset)
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

    void Buffer::WriteToIndex(const void* data, uint32_t index)
    {
        FL_ASSERT(m_BufferSpec.InstanceSize, "Failed to write to index: Instance size not specified during buffer creation!");
        WriteToBuffer(data, m_BufferSpec.InstanceSize, index * m_AlignmentSize);
    }

    VkResult Buffer::Flush(VkDeviceSize size, VkDeviceSize offset)
    {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = m_VkBufferDeviceMemory;
        mappedRange.offset = offset;
        mappedRange.size = size;

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
    }

    VkResult Buffer::FlushIndex(int index)
    {
        return Flush(m_AlignmentSize, index * m_AlignmentSize);
    }

    void Buffer::DestroyBuffer()
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