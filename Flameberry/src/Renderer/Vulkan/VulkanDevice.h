#pragma once

#include <vector>
#include <set>
#include <vulkan/vulkan.h>

#include "Core/optional.h"
#include "Renderer/Vulkan/VulkanWindow.h"

namespace Flameberry {
    struct QueueFamilyIndices
    {
        flame::optional<uint32_t> GraphicsSupportedQueueFamilyIndex;
        flame::optional<uint32_t> PresentationSupportedQueueFamilyIndex;
    };

    class VulkanDevice
    {
    public:
        VulkanDevice(VkPhysicalDevice& physicalDevice, VulkanWindow* pVulkanWindow);
        ~VulkanDevice();

        VkDevice GetVulkanDevice() const { return m_VkDevice; }
        VkQueue GetGraphicsQueue() const { return m_VkGraphicsQueue; }
        VkQueue GetPresentationQueue() const { return m_VkPresentationQueue; }
        QueueFamilyIndices GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }

        void AllocateCommandBuffers(uint32_t bufferCount);
        VkCommandBuffer GetCommandBuffer(uint32_t bufferIndex) { return m_VkCommandBuffers[bufferIndex]; }
        void ResetCommandBuffer(uint32_t bufferIndex);
        void BeginCommandBuffer(uint32_t bufferIndex, VkCommandBufferUsageFlags usageFlags = 0);
        void EndCommandBuffer(uint32_t bufferIndex);
        void BeginSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer);
        void EndSingleTimeCommandBuffer(VkCommandBuffer& commandBuffer);
        
        void WaitIdle();

        static std::shared_ptr<VulkanDevice> Create(VkPhysicalDevice& physicalDevice, VulkanWindow* pVulkanWindow) { return std::make_shared<VulkanDevice>(physicalDevice, pVulkanWindow); }
        std::vector<VkDeviceQueueCreateInfo> CreateDeviceQueueInfos(const std::set<uint32_t>& uniqueQueueFamilyIndices);
    private:
        VkDevice m_VkDevice;
        VkQueue m_VkGraphicsQueue, m_VkPresentationQueue;
        QueueFamilyIndices m_QueueFamilyIndices;

        VkCommandPool m_VkCommandPool;
        std::vector<VkCommandBuffer> m_VkCommandBuffers;

#ifdef __APPLE__
        const std::vector<const char*> m_VkDeviceExtensions = { "VK_KHR_portability_subset", VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#else
        const std::vector<const char*> m_VkDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#endif
        VkPhysicalDevice& m_VkPhysicalDevice;
    };
}
