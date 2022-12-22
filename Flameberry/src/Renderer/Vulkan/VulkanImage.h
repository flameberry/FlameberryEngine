#pragma once

#include <vulkan/vulkan.h>

namespace Flameberry {
    class VulkanImage
    {
    public:
        VulkanImage(VkDevice& device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlags imageAspectFlags);
        ~VulkanImage();

        void WriteFromBuffer(VkBuffer srcBuffer);
        void TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout);

        VkImage GetImage() const { return m_VkImage; }
        VkImageView GetImageView() const { return m_VkImageView; }
    private:
        VkImage m_VkImage;
        VkImageView m_VkImageView;
        VkDeviceMemory m_VkImageDeviceMemory;

        uint32_t m_Width, m_Height;
        VkFormat m_VkImageFormat;

        VkDevice& m_VkDevice;
    };
}