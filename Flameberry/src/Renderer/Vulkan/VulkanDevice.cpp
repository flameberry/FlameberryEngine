#include "VulkanDevice.h"

#include "VulkanDebug.h"
#include "VulkanContext.h"
#include "VulkanRenderCommand.h"

namespace Flameberry {
    VulkanDevice::VulkanDevice(VkPhysicalDevice& physicalDevice, VulkanWindow* pVulkanWindow)
        : m_VkPhysicalDevice(physicalDevice)
    {
        // Getting Queue Family Indices
        m_QueueFamilyIndices = VulkanRenderCommand::GetQueueFamilyIndices(m_VkPhysicalDevice, pVulkanWindow->GetWindowSurface());

        std::vector<VkDeviceQueueCreateInfo> vk_device_queue_create_infos = CreateDeviceQueueInfos(
            {
                m_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex,
                m_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex
            }
        );

        VkPhysicalDeviceFeatures vk_physical_device_features{};
        vk_physical_device_features.samplerAnisotropy = VK_TRUE;
        vk_physical_device_features.sampleRateShading = VK_TRUE;

        // Creating Vulkan Logical Device
        VkDeviceCreateInfo vk_device_create_info{};
        vk_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        vk_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(vk_device_queue_create_infos.size());
        vk_device_create_info.pQueueCreateInfos = vk_device_queue_create_infos.data();

        vk_device_create_info.pEnabledFeatures = &vk_physical_device_features;

        vk_device_create_info.enabledExtensionCount = static_cast<uint32_t>(m_VkDeviceExtensions.size());
        vk_device_create_info.ppEnabledExtensionNames = m_VkDeviceExtensions.data();

        auto validationLayers = VulkanContext::GetValidationLayerNames();
        if (VulkanContext::EnableValidationLayers())
        {
            vk_device_create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            vk_device_create_info.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            vk_device_create_info.enabledLayerCount = 0;
        }

        VK_CHECK_RESULT(vkCreateDevice(m_VkPhysicalDevice, &vk_device_create_info, nullptr, &m_VkDevice));

        vkGetDeviceQueue(m_VkDevice, m_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex, 0, &m_VkGraphicsQueue);
        vkGetDeviceQueue(m_VkDevice, m_QueueFamilyIndices.PresentationSupportedQueueFamilyIndex, 0, &m_VkPresentationQueue);

        // Creating Command Pool
        VkCommandPoolCreateInfo vk_command_pool_create_info{};
        vk_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        vk_command_pool_create_info.queueFamilyIndex = m_QueueFamilyIndices.GraphicsSupportedQueueFamilyIndex;
        vk_command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VK_CHECK_RESULT(vkCreateCommandPool(m_VkDevice, &vk_command_pool_create_info, nullptr, &m_VkCommandPool));
    }

    void VulkanDevice::AllocateCommandBuffers(uint32_t bufferCount)
    {
        m_VkCommandBuffers.resize(bufferCount);

        VkCommandBufferAllocateInfo vk_command_buffer_allocate_info{};
        vk_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        vk_command_buffer_allocate_info.commandPool = m_VkCommandPool;
        vk_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vk_command_buffer_allocate_info.commandBufferCount = (uint32_t)m_VkCommandBuffers.size();

        VK_CHECK_RESULT(vkAllocateCommandBuffers(m_VkDevice, &vk_command_buffer_allocate_info, m_VkCommandBuffers.data()));
    }

    void VulkanDevice::ResetCommandBuffer(uint32_t bufferIndex)
    {
        vkResetCommandBuffer(m_VkCommandBuffers[bufferIndex], 0);
    }

    void VulkanDevice::BeginCommandBuffer(uint32_t bufferIndex, VkCommandBufferUsageFlags usageFlags)
    {
        VkCommandBufferBeginInfo vk_command_buffer_begin_info{};
        vk_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vk_command_buffer_begin_info.flags = usageFlags;
        vk_command_buffer_begin_info.pInheritanceInfo = nullptr;

        VK_CHECK_RESULT(vkBeginCommandBuffer(m_VkCommandBuffers[bufferIndex], &vk_command_buffer_begin_info));
    }

    void VulkanDevice::EndCommandBuffer(uint32_t bufferIndex)
    {
        VK_CHECK_RESULT(vkEndCommandBuffer(m_VkCommandBuffers[bufferIndex]));
    }

    void VulkanDevice::BeginSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer)
    {
        VkCommandBufferAllocateInfo vk_command_buffer_allocate_info{};
        vk_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        vk_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vk_command_buffer_allocate_info.commandPool = m_VkCommandPool;
        vk_command_buffer_allocate_info.commandBufferCount = 1;

        vkAllocateCommandBuffers(m_VkDevice, &vk_command_buffer_allocate_info, &commandBuffer);

        VkCommandBufferBeginInfo vk_command_buffer_begin_info{};
        vk_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vk_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &vk_command_buffer_begin_info);
    }

    void VulkanDevice::EndSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo vk_submit_info{};
        vk_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        vk_submit_info.commandBufferCount = 1;
        vk_submit_info.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_VkGraphicsQueue, 1, &vk_submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_VkGraphicsQueue);

        vkFreeCommandBuffers(m_VkDevice, m_VkCommandPool, 1, &commandBuffer);
    }

    VulkanDevice::~VulkanDevice()
    {
        vkDestroyCommandPool(m_VkDevice, m_VkCommandPool, nullptr);
        vkDestroyDevice(m_VkDevice, nullptr);
    }

    std::vector<VkDeviceQueueCreateInfo> VulkanDevice::CreateDeviceQueueInfos(const std::set<uint32_t>& uniqueQueueFamilyIndices)
    {
        float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> vk_device_queue_create_infos;
        for (uint32_t uniqueQueueFamilyIndex : uniqueQueueFamilyIndices)
        {
            VkDeviceQueueCreateInfo vk_queue_create_info{};
            vk_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            vk_queue_create_info.queueFamilyIndex = uniqueQueueFamilyIndex;
            vk_queue_create_info.queueCount = 1;
            vk_queue_create_info.pQueuePriorities = &queuePriority;
            vk_device_queue_create_infos.push_back(vk_queue_create_info);
        }
        return vk_device_queue_create_infos;
    }

    void VulkanDevice::WaitIdle()
    {
        vkDeviceWaitIdle(m_VkDevice);
    }

}
