#include "Image.h"

#include "VulkanDebug.h"
#include "RenderCommand.h"
#include "VulkanContext.h"

namespace Flameberry {

	Image::Image(const ImageSpecification& specification)
		: m_Specification(specification), m_ReferenceCount(new uint32_t(1))
	{
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		const auto& physicalDevice = VulkanContext::GetPhysicalDevice();

		// Creating Image
		VkImageCreateInfo vk_image_create_info{};
		vk_image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		vk_image_create_info.imageType = VK_IMAGE_TYPE_2D;
		vk_image_create_info.extent.width = m_Specification.Width;
		vk_image_create_info.extent.height = m_Specification.Height;
		vk_image_create_info.extent.depth = 1;
		vk_image_create_info.mipLevels = m_Specification.MipLevels;
		vk_image_create_info.arrayLayers = m_Specification.ArrayLayers;
		vk_image_create_info.format = m_Specification.Format;
		vk_image_create_info.tiling = m_Specification.Tiling;
		vk_image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vk_image_create_info.usage = m_Specification.Usage;
		vk_image_create_info.samples = (VkSampleCountFlagBits)m_Specification.Samples;
		vk_image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vk_image_create_info.flags = m_Specification.Flags;

		VK_CHECK_RESULT(vkCreateImage(device, &vk_image_create_info, nullptr, &m_VkImage));

		vkGetImageMemoryRequirements(device, m_VkImage, &m_MemoryRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = m_MemoryRequirements.size;
		allocInfo.memoryTypeIndex = RenderCommand::GetValidMemoryTypeIndex(physicalDevice, m_MemoryRequirements.memoryTypeBits, m_Specification.MemoryProperties);

		VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &m_VkImageDeviceMemory));
		vkBindImageMemory(device, m_VkImage, m_VkImageDeviceMemory, 0);

		// Creating Image View
		VkImageViewCreateInfo vk_image_view_create_info{};
		vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		vk_image_view_create_info.image = m_VkImage;

		// This line is really weird
		vk_image_view_create_info.viewType = m_Specification.ViewSpecification.ViewType == VK_IMAGE_VIEW_TYPE_2D && m_Specification.ViewSpecification.LayerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : m_Specification.ViewSpecification.ViewType;

		vk_image_view_create_info.format = m_Specification.Format;
		vk_image_view_create_info.subresourceRange.aspectMask = m_Specification.ViewSpecification.AspectFlags;
		vk_image_view_create_info.subresourceRange.baseMipLevel = m_Specification.ViewSpecification.BaseMipLevel;
		vk_image_view_create_info.subresourceRange.levelCount = m_Specification.MipLevels;
		vk_image_view_create_info.subresourceRange.baseArrayLayer = m_Specification.ViewSpecification.BaseArrayLayer;
		vk_image_view_create_info.subresourceRange.layerCount = m_Specification.ViewSpecification.LayerCount;
		vk_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		vk_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		vk_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		vk_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		VK_CHECK_RESULT(vkCreateImageView(device, &vk_image_view_create_info, nullptr, &m_VkImageView));
	}

	Image::Image(const Ref<Image>& image, const ImageViewSpecification& viewSpecification)
		: m_VkImage(image->m_VkImage), m_VkImageDeviceMemory(image->m_VkImageDeviceMemory), m_Specification(image->m_Specification), m_ReferenceCount(image->m_ReferenceCount)
	{
		m_Specification.ViewSpecification = viewSpecification;

		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VkImageViewCreateInfo vk_image_view_create_info{};
		vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		vk_image_view_create_info.image = m_VkImage;
		vk_image_view_create_info.viewType = m_Specification.ViewSpecification.LayerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		vk_image_view_create_info.format = m_Specification.Format;
		vk_image_view_create_info.subresourceRange.aspectMask = m_Specification.ViewSpecification.AspectFlags;
		vk_image_view_create_info.subresourceRange.baseMipLevel = m_Specification.ViewSpecification.BaseMipLevel;
		vk_image_view_create_info.subresourceRange.levelCount = m_Specification.MipLevels;
		vk_image_view_create_info.subresourceRange.baseArrayLayer = m_Specification.ViewSpecification.BaseArrayLayer;
		vk_image_view_create_info.subresourceRange.layerCount = m_Specification.ViewSpecification.LayerCount;
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

	void Image::GenerateMipmaps(VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		const auto& device = VulkanContext::GetCurrentDevice();
		VkCommandBuffer cmdBuffer;
		device->BeginSingleTimeCommandBuffer(cmdBuffer);
		CmdGenerateMipmaps(cmdBuffer, oldLayout, newLayout);
		device->EndSingleTimeCommandBuffer(cmdBuffer);
	}

	void Image::CmdGenerateMipmaps(VkCommandBuffer cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(VulkanContext::GetPhysicalDevice(), m_Specification.Format, &formatProperties);
		FBY_ASSERT(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, "Texture Image Format does not support linear blitting!");

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_VkImage;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = m_Specification.ArrayLayers;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = m_Specification.Width;
		int32_t mipHeight = m_Specification.Height;

		for (uint32_t i = 1; i < m_Specification.MipLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = m_Specification.ArrayLayers;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = m_Specification.ArrayLayers;

			vkCmdBlitImage(cmdBuffer,
				m_VkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				m_VkImage, oldLayout,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = newLayout;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1)
				mipWidth /= 2;
			if (mipHeight > 1)
				mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = m_Specification.MipLevels - 1;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
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
		vk_buffer_image_copy_region.imageExtent = { m_Specification.Width, m_Specification.Height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, srcBuffer, m_VkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_buffer_image_copy_region);
		device->EndSingleTimeCommandBuffer(commandBuffer);
	}

	void Image::TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask)
	{
		const auto& device = VulkanContext::GetCurrentDevice();
		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

		device->BeginSingleTimeCommandBuffer(cmdBuffer);
		CmdTransitionLayout(cmdBuffer, oldLayout, newLayout, aspectMask);
		device->EndSingleTimeCommandBuffer(cmdBuffer);
	}

	void Image::CmdTransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask)
	{
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = m_Specification.MipLevels;
		subresourceRange.layerCount = m_Specification.ArrayLayers;

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
			default:
				break;
		}

		// Put barrier inside setup command buffer
		vkCmdPipelineBarrier(cmdBuffer,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);
	}

} // namespace Flameberry
