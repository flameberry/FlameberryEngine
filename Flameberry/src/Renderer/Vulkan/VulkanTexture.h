#pragma once

#include <vulkan/vulkan.h>
#include "VulkanImage.h"

#include <memory>

namespace Flameberry {
    class VulkanTexture
    {
    public:
        VulkanTexture(VkDevice& device, const char* texturePath);
        ~VulkanTexture();

        VkImageView GetImageView() const { return m_TextureImage->GetImageView(); }
        VkSampler GetSampler() const { return m_VkTextureSampler; }
    private:
        std::unique_ptr<VulkanImage> m_TextureImage;
        VkSampler m_VkTextureSampler;

        VkDevice& m_VkDevice;
    };
}
