#pragma once

#include <memory>
#include <vulkan/vulkan.h>

namespace Flameberry {
    struct ImageSpecification
    {
        uint32_t Width, Height;
        uint32_t MipLevels, Samples;
        VkFormat Format;
        VkImageTiling Tiling;
        VkImageUsageFlags Usage;
        VkMemoryPropertyFlags MemoryProperties;
        VkImageAspectFlags ImageAspectFlags;
    };

    class Image
    {
    public:
        Image(const ImageSpecification& specification);
        ~Image();

        void GenerateMipMaps();
        void WriteFromBuffer(VkBuffer srcBuffer);
        void TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout);

        VkImage GetImage() const { return m_VkImage; }
        VkImageView GetImageView() const { return m_VkImageView; }

        ImageSpecification GetSpecification() const { return m_ImageSpec; }

        template<typename... Args>
        static std::shared_ptr<Image> Create(Args... args) { return std::make_shared<Image>(std::forward<Args>(args)...); }
    private:
        VkImage m_VkImage;
        VkImageView m_VkImageView;
        VkDeviceMemory m_VkImageDeviceMemory;

        ImageSpecification m_ImageSpec;
    };
}
