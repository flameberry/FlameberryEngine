#include "Image.h"

#include "VulkanDebug.h"
#include "RenderCommand.h"
#include "VulkanContext.h"

namespace Flameberry {
    Image::Image(const ImageSpecification& specification)
        : m_ImageSpec(specification), m_ReferenceCount(new uint32_t(1))
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        const auto& physicalDevice = VulkanContext::GetPhysicalDevice();

        // Creating Image
        VkImageCreateInfo vk_image_create_info{};
        vk_image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        vk_image_create_info.imageType = VK_IMAGE_TYPE_2D;
        vk_image_create_info.extent.width = m_ImageSpec.Width;
        vk_image_create_info.extent.height = m_ImageSpec.Height;
        vk_image_create_info.extent.depth = 1;
        vk_image_create_info.mipLevels = m_ImageSpec.MipLevels;
        vk_image_create_info.arrayLayers = m_ImageSpec.ArrayLayers;
        vk_image_create_info.format = m_ImageSpec.Format;
        vk_image_create_info.tiling = m_ImageSpec.Tiling;
        vk_image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vk_image_create_info.usage = m_ImageSpec.Usage;
        vk_image_create_info.samples = (VkSampleCountFlagBits)m_ImageSpec.Samples;
        vk_image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK_RESULT(vkCreateImage(device, &vk_image_create_info, nullptr, &m_VkImage));

        vkGetImageMemoryRequirements(device, m_VkImage, &m_MemoryRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = m_MemoryRequirements.size;
        allocInfo.memoryTypeIndex = RenderCommand::GetValidMemoryTypeIndex(physicalDevice, m_MemoryRequirements.memoryTypeBits, m_ImageSpec.MemoryProperties);

        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &m_VkImageDeviceMemory));
        vkBindImageMemory(device, m_VkImage, m_VkImageDeviceMemory, 0);

        // Creating Image View
        VkImageViewCreateInfo vk_image_view_create_info{};
        vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vk_image_view_create_info.image = m_VkImage;
        vk_image_view_create_info.viewType = m_ImageSpec.ViewSpecification.LayerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        vk_image_view_create_info.format = m_ImageSpec.Format;
        vk_image_view_create_info.subresourceRange.aspectMask = m_ImageSpec.ViewSpecification.AspectFlags;
        vk_image_view_create_info.subresourceRange.baseMipLevel = m_ImageSpec.ViewSpecification.BaseMipLevel;
        vk_image_view_create_info.subresourceRange.levelCount = m_ImageSpec.MipLevels;
        vk_image_view_create_info.subresourceRange.baseArrayLayer = m_ImageSpec.ViewSpecification.BaseArrayLayer;
        vk_image_view_create_info.subresourceRange.layerCount = m_ImageSpec.ViewSpecification.LayerCount;
        vk_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        vk_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        vk_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        vk_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        VK_CHECK_RESULT(vkCreateImageView(device, &vk_image_view_create_info, nullptr, &m_VkImageView));
    }

    Image::Image(const std::shared_ptr<Image>& image, const ImageViewSpecification& viewSpecification)
        : m_VkImage(image->m_VkImage), m_VkImageDeviceMemory(image->m_VkImageDeviceMemory), m_ImageSpec(image->m_ImageSpec), m_ReferenceCount(image->m_ReferenceCount)
    {
        m_ImageSpec.ViewSpecification = viewSpecification;

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VkImageViewCreateInfo vk_image_view_create_info{};
        vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vk_image_view_create_info.image = m_VkImage;
        vk_image_view_create_info.viewType = m_ImageSpec.ViewSpecification.LayerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        vk_image_view_create_info.format = m_ImageSpec.Format;
        vk_image_view_create_info.subresourceRange.aspectMask = m_ImageSpec.ViewSpecification.AspectFlags;
        vk_image_view_create_info.subresourceRange.baseMipLevel = m_ImageSpec.ViewSpecification.BaseMipLevel;
        vk_image_view_create_info.subresourceRange.levelCount = m_ImageSpec.MipLevels;
        vk_image_view_create_info.subresourceRange.baseArrayLayer = m_ImageSpec.ViewSpecification.BaseArrayLayer;
        vk_image_view_create_info.subresourceRange.layerCount = m_ImageSpec.ViewSpecification.LayerCount;
        vk_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        vk_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        vk_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        vk_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        VK_CHECK_RESULT(vkCreateImageView(device, &vk_image_view_create_info, nullptr, &m_VkImageView));

        (*m_ReferenceCount)++;
    }

    Image::~Image()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyImageView(device, m_VkImageView, nullptr);
        if (--(*m_ReferenceCount) == 0)
        {
            vkDestroyImage(device, m_VkImage, nullptr);
            vkFreeMemory(device, m_VkImageDeviceMemory, nullptr);
            delete m_ReferenceCount;
        }
    }

    void Image::GenerateMipMaps()
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(VulkanContext::GetPhysicalDevice(), m_ImageSpec.Format, &formatProperties);
        FBY_ASSERT(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, "Texture Image Format does not support linear blitting!");

        const auto& device = VulkanContext::GetCurrentDevice();
        VkCommandBuffer commandBuffer;
        device->BeginSingleTimeCommandBuffer(commandBuffer);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = m_VkImage;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = m_ImageSpec.Width;
        int32_t mipHeight = m_ImageSpec.Height;

        for (uint32_t i = 1; i < m_ImageSpec.MipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                m_VkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                m_VkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR
            );

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            if (mipWidth > 1)
                mipWidth /= 2;
            if (mipHeight > 1)
                mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = m_ImageSpec.MipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        device->EndSingleTimeCommandBuffer(commandBuffer);
    }

    void Image::WriteFromBuffer(VkBuffer srcBuffer)
    {
        const auto& device = VulkanContext::GetCurrentDevice();

        VkCommandBuffer commandBuffer;
        device->BeginSingleTimeCommandBuffer(commandBuffer);

        VkBufferImageCopy vk_buffer_image_copy_region{};
        vk_buffer_image_copy_region.bufferOffset = 0;
        vk_buffer_image_copy_region.bufferRowLength = 0;
        vk_buffer_image_copy_region.bufferImageHeight = 0;

        vk_buffer_image_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vk_buffer_image_copy_region.imageSubresource.mipLevel = 0;
        vk_buffer_image_copy_region.imageSubresource.baseArrayLayer = 0;
        vk_buffer_image_copy_region.imageSubresource.layerCount = 1;

        vk_buffer_image_copy_region.imageOffset = { 0, 0, 0 };
        vk_buffer_image_copy_region.imageExtent = { m_ImageSpec.Width, m_ImageSpec.Height, 1 };

        vkCmdCopyBufferToImage(commandBuffer, srcBuffer, m_VkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_buffer_image_copy_region);
        device->EndSingleTimeCommandBuffer(commandBuffer);
    }

    void Image::TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask)
    {
        const auto& device = VulkanContext::GetCurrentDevice();
        
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = aspectMask;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = m_ImageSpec.MipLevels;
        subresourceRange.layerCount = 1;
        
        // Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.oldLayout = oldLayout;
        imageMemoryBarrier.newLayout = newLayout;
        imageMemoryBarrier.image = m_VkImage;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        
        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (oldLayout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                // Image layout is undefined (or does not matter)
                // Only valid as initial layout
                // No flags required, listed only for completeness
                imageMemoryBarrier.srcAccessMask = 0;
                break;
                
            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                // Image is preinitialized
                // Only valid as initial layout for linear images, preserves memory contents
                // Make sure host writes have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;
                
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image is a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
                
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image is a depth/stencil attachment
                // Make sure any writes to the depth/stencil buffer have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
                
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image is a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;
                
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image is a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
                
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image is read by a shader
                // Make sure any shader reads from the image have been finished
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }
        
        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newLayout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                // Image will be used as a transfer destination
                // Make sure any writes to the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
                
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                // Image will be used as a transfer source
                // Make sure any reads from the image have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;
                
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Image will be used as a color attachment
                // Make sure any writes to the color buffer have been finished
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
                
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                // Image layout will be used as a depth/stencil attachment
                // Make sure any writes to depth/stencil buffer have been finished
                imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
                
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                // Image will be read in a shader (sampler, input attachment)
                // Make sure any writes to the image have been finished
                if (imageMemoryBarrier.srcAccessMask == 0)
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default: break;
        }
        
        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
        device->BeginSingleTimeCommandBuffer(cmdBuffer);
        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(cmdBuffer,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &imageMemoryBarrier);
        device->EndSingleTimeCommandBuffer(cmdBuffer);
    }
}
