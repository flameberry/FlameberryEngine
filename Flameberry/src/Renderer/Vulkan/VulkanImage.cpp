#include "VulkanImage.h"

#include "VulkanDebug.h"
#include "VulkanRenderCommand.h"
#include "VulkanContext.h"

namespace Flameberry {
    VulkanImage::VulkanImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImageAspectFlags imageAspectFlags,
        VkSampleCountFlagBits sampleCount
    ) : m_Width(width), m_Height(height), m_VkImageFormat(format)
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        const auto& physicalDevice = VulkanContext::GetPhysicalDevice();

        // Creating Image
        VkImageCreateInfo vk_image_create_info{};
        vk_image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        vk_image_create_info.imageType = VK_IMAGE_TYPE_2D;
        vk_image_create_info.extent.width = width;
        vk_image_create_info.extent.height = height;
        vk_image_create_info.extent.depth = 1;
        vk_image_create_info.mipLevels = 1;
        vk_image_create_info.arrayLayers = 1;
        vk_image_create_info.format = format;
        vk_image_create_info.tiling = tiling;
        vk_image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vk_image_create_info.usage = usage;
        vk_image_create_info.samples = sampleCount;
        vk_image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK_RESULT(vkCreateImage(device, &vk_image_create_info, nullptr, &m_VkImage));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, m_VkImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VulkanRenderCommand::GetValidMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, properties);

        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &m_VkImageDeviceMemory));
        vkBindImageMemory(device, m_VkImage, m_VkImageDeviceMemory, 0);

        // Creating Image View
        VkImageViewCreateInfo vk_image_view_create_info{};
        vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vk_image_view_create_info.image = m_VkImage;
        vk_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vk_image_view_create_info.format = format;
        vk_image_view_create_info.subresourceRange.aspectMask = imageAspectFlags;
        vk_image_view_create_info.subresourceRange.baseMipLevel = 0;
        vk_image_view_create_info.subresourceRange.levelCount = 1;
        vk_image_view_create_info.subresourceRange.baseArrayLayer = 0;
        vk_image_view_create_info.subresourceRange.layerCount = 1;

        VK_CHECK_RESULT(vkCreateImageView(device, &vk_image_view_create_info, nullptr, &m_VkImageView));
    }

    VulkanImage::~VulkanImage()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyImageView(device, m_VkImageView, nullptr);
        vkDestroyImage(device, m_VkImage, nullptr);
        vkFreeMemory(device, m_VkImageDeviceMemory, nullptr);
    }

    void VulkanImage::WriteFromBuffer(VkBuffer srcBuffer)
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
        vk_buffer_image_copy_region.imageExtent = { m_Width, m_Height, 1 };

        vkCmdCopyBufferToImage(commandBuffer, srcBuffer, m_VkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_buffer_image_copy_region);
        device->EndSingleTimeCommandBuffer(commandBuffer);
    }

    void VulkanImage::TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        const auto& device = VulkanContext::GetCurrentDevice();

        VkCommandBuffer commandBuffer;
        device->BeginSingleTimeCommandBuffer(commandBuffer);

        VkPipelineStageFlags sourceStageFlags;
        VkPipelineStageFlags destinationStageFlags;

        VkImageMemoryBarrier vk_image_memory_barrier{};

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            vk_image_memory_barrier.srcAccessMask = 0;
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            vk_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
            FL_ERROR("Unsupported Image Layout Transition!");

        vk_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        vk_image_memory_barrier.oldLayout = oldLayout;
        vk_image_memory_barrier.newLayout = newLayout;
        vk_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_image_memory_barrier.image = m_VkImage;
        vk_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vk_image_memory_barrier.subresourceRange.baseMipLevel = 0;
        vk_image_memory_barrier.subresourceRange.levelCount = 1;
        vk_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        vk_image_memory_barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(commandBuffer, sourceStageFlags, destinationStageFlags, 0, 0, nullptr, 0, nullptr, 1, &vk_image_memory_barrier);

        device->EndSingleTimeCommandBuffer(commandBuffer);
    }
}