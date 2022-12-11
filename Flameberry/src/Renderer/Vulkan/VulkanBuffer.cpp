#include "VulkanBuffer.h"

#include "Core/Log.h"

#include "VulkanRenderer.h"

namespace Flameberry {
    VulkanBuffer::VulkanBuffer(VkDevice& deviceInstance, VkDeviceSize deviceSize, const void* data)
        : m_VkDevice(deviceInstance)
    {
        VkDeviceSize bufferSize = deviceSize;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        VulkanRenderer::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* vk_vertex_buffer_data;
        vkMapMemory(m_VkDevice, stagingBufferMemory, 0, bufferSize, 0, &vk_vertex_buffer_data);
        memcpy(vk_vertex_buffer_data, data, (size_t)bufferSize);
        vkUnmapMemory(m_VkDevice, stagingBufferMemory);

        VulkanRenderer::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VkBuffer, m_VkBufferDeviceMemory);
        VulkanRenderer::CopyBuffer(stagingBuffer, m_VkBuffer, bufferSize);

        vkDestroyBuffer(m_VkDevice, stagingBuffer, nullptr);
        vkFreeMemory(m_VkDevice, stagingBufferMemory, nullptr);
    }

    VulkanBuffer::~VulkanBuffer()
    {
        vkDestroyBuffer(m_VkDevice, m_VkBuffer, nullptr);
        FL_INFO("Destroyed Vulkan Vertex Buffer!");

        vkFreeMemory(m_VkDevice, m_VkBufferDeviceMemory, nullptr);
        FL_INFO("Freed Vulkan Vertex Buffer Device Memory!");
    }
}