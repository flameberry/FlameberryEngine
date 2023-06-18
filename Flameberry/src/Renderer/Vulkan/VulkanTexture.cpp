#include "VulkanTexture.h"

#include "VulkanDebug.h"
#include "Buffer.h"

#include "VulkanContext.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <filesystem>

namespace Flameberry {
    std::shared_ptr<DescriptorSetLayout> VulkanTexture::s_DescriptorLayout;
    std::shared_ptr<DescriptorSet> VulkanTexture::s_EmptyDescriptorSet;
    std::shared_ptr<Image> VulkanTexture::s_EmptyImage;
    VkSampler VulkanTexture::s_DefaultSampler;

    std::unordered_map<std::string, std::shared_ptr<VulkanTexture>> VulkanTexture::s_TextureCacheDirectory;

    VulkanTexture::VulkanTexture(const std::string& texturePath, VkSampler sampler)
        : m_FilePath(texturePath)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        int width, height, channels, bytes_per_channel;

        void* pixels = nullptr;
        uint32_t mipLevels = 1;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkDeviceSize imageSize = 0;

        auto file = std::filesystem::path(texturePath);
        if (file.extension() == ".hdr")
        {
            pixels = stbi_loadf(texturePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            mipLevels = 1;
            format = VK_FORMAT_R32G32B32A32_SFLOAT;
            bytes_per_channel = 4;
        }
        else
        {
            pixels = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
            format = VK_FORMAT_R8G8B8A8_UNORM;
            bytes_per_channel = 1;
        }

        FL_ASSERT(pixels, "Texture pixels are empty!");
        imageSize = 4 * width * height * bytes_per_channel;

        m_TextureImageSpecification.Width = width;
        m_TextureImageSpecification.Height = height;
        m_TextureImageSpecification.MipLevels = mipLevels;
        m_TextureImageSpecification.Samples = 1;
        m_TextureImageSpecification.Format = format;
        m_TextureImageSpecification.Tiling = VK_IMAGE_TILING_OPTIMAL;
        m_TextureImageSpecification.Usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        m_TextureImageSpecification.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        m_TextureImageSpecification.ImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

        m_TextureImage = Image::Create(m_TextureImageSpecification);

        BufferSpecification stagingBufferSpec;
        stagingBufferSpec.InstanceCount = 1;
        stagingBufferSpec.InstanceSize = imageSize;
        stagingBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        Buffer stagingBuffer(stagingBufferSpec);

        stagingBuffer.MapMemory(imageSize);
        stagingBuffer.WriteToBuffer(pixels, imageSize, 0);
        stagingBuffer.UnmapMemory();

        stbi_image_free(pixels);

        m_TextureImage->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        m_TextureImage->WriteFromBuffer(stagingBuffer.GetBuffer());

        if (m_TextureImageSpecification.MipLevels > 1)
            m_TextureImage->GenerateMipMaps();
        else
            m_TextureImage->TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        if (sampler == VK_NULL_HANDLE)
        {
            m_DidCreateSampler = true;

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
            sampler_info.maxLod = (float)mipLevels;

            VK_CHECK_RESULT(vkCreateSampler(device, &sampler_info, nullptr, &m_VkTextureSampler));
        }
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
        if (m_DidCreateSampler)
            vkDestroySampler(device, m_VkTextureSampler, nullptr);
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
            FL_WARN("Texture not found in cache! Loading texture from disk!");
            return texture;
        }
    }

    void VulkanTexture::InitStaticResources()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

        ImageSpecification imageSpec;
        imageSpec.Width = 1;
        imageSpec.Height = 1;
        imageSpec.MipLevels = 1;
        imageSpec.Samples = 1;
        imageSpec.Format = VK_FORMAT_R8G8B8A8_SRGB;
        imageSpec.Tiling = VK_IMAGE_TILING_OPTIMAL;
        imageSpec.Usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        imageSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageSpec.ImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

        s_EmptyImage = Image::Create(imageSpec);

        {
            const auto& device = VulkanContext::GetCurrentDevice();

            VkCommandBuffer commandBuffer;
            device->BeginSingleTimeCommandBuffer(commandBuffer);

            VkPipelineStageFlags sourceStageFlags;
            VkPipelineStageFlags destinationStageFlags;

            VkImageMemoryBarrier vk_image_memory_barrier{};
            vk_image_memory_barrier.srcAccessMask = 0;
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            vk_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            vk_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            vk_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            vk_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_image_memory_barrier.image = s_EmptyImage->GetImage();
            vk_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vk_image_memory_barrier.subresourceRange.baseMipLevel = 0;
            vk_image_memory_barrier.subresourceRange.levelCount = 1;
            vk_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
            vk_image_memory_barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(commandBuffer, sourceStageFlags, destinationStageFlags, 0, 0, nullptr, 0, nullptr, 1, &vk_image_memory_barrier);

            device->EndSingleTimeCommandBuffer(commandBuffer);
        }

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

        DescriptorSetLayoutSpecification emptyDescSetLayoutSpec;
        emptyDescSetLayoutSpec.Bindings.emplace_back();
        emptyDescSetLayoutSpec.Bindings[0].binding = 0;
        emptyDescSetLayoutSpec.Bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        emptyDescSetLayoutSpec.Bindings[0].descriptorCount = 1;
        emptyDescSetLayoutSpec.Bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        emptyDescSetLayoutSpec.Bindings[0].pImmutableSamplers = nullptr;

        s_DescriptorLayout = DescriptorSetLayout::Create(emptyDescSetLayoutSpec);

        DescriptorSetSpecification emptyDescSetSpec;
        emptyDescSetSpec.Layout = s_DescriptorLayout;

        s_EmptyDescriptorSet = DescriptorSet::Create(emptyDescSetSpec);

        VkDescriptorImageInfo desc_image{};
        desc_image.sampler = s_DefaultSampler;
        desc_image.imageView = s_EmptyImage->GetImageView();
        desc_image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        s_EmptyDescriptorSet->WriteImage(0, desc_image);
        s_EmptyDescriptorSet->Update();
    }

    void VulkanTexture::DestroyStaticResources()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroySampler(device, s_DefaultSampler, nullptr);

        s_EmptyDescriptorSet = nullptr;
        s_DescriptorLayout = nullptr;
        s_EmptyImage = nullptr;

        s_TextureCacheDirectory.clear();
    }
}
