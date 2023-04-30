#pragma once

#include <memory>
#include <vulkan/vulkan.h>

namespace Flameberry {
    class VulkanImage
    {
    public:
        VulkanImage(
            uint32_t width,
            uint32_t height,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImageAspectFlags imageAspectFlags,
            VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT
        );
        ~VulkanImage();

        void WriteFromBuffer(VkBuffer srcBuffer);
        void TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout);

        VkImage GetImage() const { return m_VkImage; }
        VkImageView GetImageView() const { return m_VkImageView; }

        template<typename... Args>
        static std::shared_ptr<VulkanImage> Create(Args... args) { return std::make_shared<VulkanImage>(std::forward<Args>(args)...); }
    private:
        VkImage m_VkImage;
        VkImageView m_VkImageView;
        VkDeviceMemory m_VkImageDeviceMemory;

        uint32_t m_Width, m_Height;
        VkFormat m_VkImageFormat;
    };
}
