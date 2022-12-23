#include "VulkanTexture.h"

#include "Core/Core.h"
#include "VulkanBuffer.h"

#include "VulkanRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

namespace Flameberry {
    VulkanTexture::VulkanTexture(VkDevice& device, const char* texturePath)
        : m_VkDevice(device)
    {
        int width, height, channels;
        stbi_uc* pixels = stbi_load(texturePath, &width, &height, &channels, STBI_rgb_alpha);
        FL_ASSERT(pixels, "Texture pixels are empty!");

        VkDeviceSize imageSize = 4 * width * height;
        VulkanBuffer stagingBuffer(m_VkDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        stagingBuffer.MapMemory(imageSize);
        stagingBuffer.WriteToBuffer(pixels, imageSize, 0);
        stagingBuffer.UnmapMemory();

        stbi_image_free(pixels);

        m_TextureImage = std::make_unique<VulkanImage>(
            m_VkDevice,
            width,
            height,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
            );

        m_TextureImage->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        m_TextureImage->WriteFromBuffer(stagingBuffer.GetBuffer());
        m_TextureImage->TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Create Sampler
        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.anisotropyEnable = VK_TRUE;

        sampler_info.maxAnisotropy = VulkanRenderer::GetPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 0.0f;

        FL_ASSERT(vkCreateSampler(m_VkDevice, &sampler_info, nullptr, &m_VkTextureSampler) == VK_SUCCESS, "Failed to create Vulkan Texture Sampler!");
    }

    VulkanTexture::~VulkanTexture()
    {
        vkDestroySampler(m_VkDevice, m_VkTextureSampler, nullptr);
    }
}
