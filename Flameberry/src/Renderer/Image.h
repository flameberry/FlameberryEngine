#pragma once

#include <memory>
#include <vulkan/vulkan.h>

#include "Core/Core.h"

namespace Flameberry {
    struct ImageViewSpecification
    {
        VkImageAspectFlags AspectFlags;
        uint32_t BaseMipLevel = 0, BaseArrayLayer = 0, LayerCount = 1;
    };

    struct ImageSpecification
    {
        uint32_t Width, Height;
        uint32_t Samples = 1, MipLevels = 1, ArrayLayers = 1;
        VkFormat Format;
        VkImageTiling Tiling;
        VkImageUsageFlags Usage;
        VkMemoryPropertyFlags MemoryProperties;
        ImageViewSpecification ViewSpecification;
    };

    class Image
    {
    public:
        Image(const ImageSpecification& specification);
        Image(const Ref<Image>& image, const ImageViewSpecification& viewSpecification);
        ~Image();

        void GenerateMipMaps();
        void WriteFromBuffer(VkBuffer srcBuffer);
        void TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

        VkImage GetImage() const { return m_VkImage; }
        VkImageView GetImageView() const { return m_VkImageView; }

        ImageSpecification GetSpecification() const { return m_ImageSpec; }
        VkMemoryRequirements GetMemoryRequirements() const { return m_MemoryRequirements; }
    private:
        VkImage m_VkImage;
        VkImageView m_VkImageView;
        VkDeviceMemory m_VkImageDeviceMemory;

        VkMemoryRequirements m_MemoryRequirements;
        ImageSpecification m_ImageSpec;

        uint32_t* m_ReferenceCount;
    };
}
