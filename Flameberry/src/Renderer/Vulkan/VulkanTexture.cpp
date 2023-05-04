#include "VulkanTexture.h"

#include "VulkanDebug.h"
#include "VulkanBuffer.h"

#include "VulkanRenderer.h"
#include "VulkanContext.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

namespace Flameberry {
    std::shared_ptr<VulkanDescriptorLayout> VulkanTexture::s_DescriptorLayout;
    VkDescriptorSet VulkanTexture::s_EmptyDescriptorSet;
    std::shared_ptr<VulkanImage> VulkanTexture::s_EmptyImage;
    VkSampler VulkanTexture::s_DefaultSampler;

    std::unordered_map<std::string, std::shared_ptr<VulkanTexture>> VulkanTexture::s_TextureCacheDirectory;

    VulkanTexture::VulkanTexture(const char* texturePath, VkSampler sampler)
        : m_FilePath(texturePath)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        int width, height, channels;
        stbi_uc* pixels = stbi_load(texturePath, &width, &height, &channels, STBI_rgb_alpha);
        FL_ASSERT(pixels, "Texture pixels are empty!");

        VkDeviceSize imageSize = 4 * width * height;
        VulkanBuffer stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        stagingBuffer.MapMemory(imageSize);
        stagingBuffer.WriteToBuffer(pixels, imageSize, 0);
        stagingBuffer.UnmapMemory();

        stbi_image_free(pixels);

        m_TextureImage = std::make_unique<VulkanImage>(
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

        if (sampler == VK_NULL_HANDLE)
            m_VkTextureSampler = s_DefaultSampler;
        else
            m_VkTextureSampler = sampler;

        // Create Descriptor Set for the Texture
        {
            VulkanContext::GetCurrentGlobalDescriptorPool()->AllocateDescriptorSet(&m_DescriptorSet, s_DescriptorLayout->GetLayout());

            // Update the Descriptor Set:
            VkDescriptorImageInfo desc_image[1] = {};
            desc_image[0].sampler = m_VkTextureSampler;
            desc_image[0].imageView = m_TextureImage->GetImageView();
            desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet write_desc[1] = {};
            write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_desc[0].dstSet = m_DescriptorSet;
            write_desc[0].descriptorCount = 1;
            write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write_desc[0].pImageInfo = desc_image;
            vkUpdateDescriptorSets(device, 1, write_desc, 0, nullptr);
        }
    }

    VulkanTexture::~VulkanTexture()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkFreeDescriptorSets(device, VulkanContext::GetCurrentGlobalDescriptorPool()->GetVulkanDescriptorPool(), 1, &m_DescriptorSet);
    }

    std::shared_ptr<VulkanTexture> VulkanTexture::TryGetOrLoadTexture(const std::string& texturePath)
    {
        if (s_TextureCacheDirectory.find(texturePath) != s_TextureCacheDirectory.end()) {
            FL_INFO("Successfully retrieved vulkan texture from cache!");
            return s_TextureCacheDirectory[texturePath];
        }
        else {
            std::shared_ptr<VulkanTexture> texture = std::make_shared<VulkanTexture>(texturePath.c_str());
            s_TextureCacheDirectory[texturePath] = texture;
            FL_INFO("Texture not found in cache! Loading texture from disk!");
            return texture;
        }
    }

    void VulkanTexture::InitStaticResources()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        s_EmptyImage = std::make_shared<VulkanImage>(
            1,
            1,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );

        // Create Sampler
        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.anisotropyEnable = VK_TRUE;

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(VulkanContext::GetPhysicalDevice(), &properties);

        sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 0.0f;

        VK_CHECK_RESULT(vkCreateSampler(device, &sampler_info, nullptr, &s_DefaultSampler));

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayoutBinding.pImmutableSamplers = nullptr;

        s_DescriptorLayout = std::make_shared<VulkanDescriptorLayout>(std::vector<VkDescriptorSetLayoutBinding>{ samplerLayoutBinding });

        VulkanContext::GetCurrentGlobalDescriptorPool()->AllocateDescriptorSet(&s_EmptyDescriptorSet, s_DescriptorLayout->GetLayout());

        // Update the Descriptor Set:
        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = s_DefaultSampler;
        desc_image[0].imageView = s_EmptyImage->GetImageView();
        desc_image[0].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkWriteDescriptorSet write_desc[1] = {};
        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = s_EmptyDescriptorSet;
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc[0].pImageInfo = desc_image;
        vkUpdateDescriptorSets(device, 1, write_desc, 0, nullptr);
    }

    void VulkanTexture::DestroyStaticResources()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroySampler(device, s_DefaultSampler, nullptr);

        s_DescriptorLayout = nullptr;
        s_EmptyImage = nullptr;

        s_TextureCacheDirectory.clear();
    }
}
