#include "Image.h"

#include "Core/Assert.h"
#include "Core/Log.h"
#include "VulkanDebug.h"
#include "RenderCommand.h"
#include "VulkanContext.h"
#include "vulkan/vulkan_core.h"

namespace Flameberry {

	VkImageView Image::GetVulkanImageView(int idx) const
	{
		switch (m_Specification.ViewCreationMode)
		{
			case ImageViewCreationMode::CreateUsingGivenSpecification:
				if (idx > 0)
					FBY_WARN("Cannot access image view at index: {} - Image was created with ImageViewCreationMode::CreateUsingGivenSpecification mode.", idx);
				return m_VulkanImageView;

			case ImageViewCreationMode::CreateOnePerMipMap:
				if (idx >= m_Specification.MipLevels)
					FBY_ERROR("Cannot access image view at index: {} - Index out of bounds (Image views total: {}).", idx, m_VulkanImageViewVector.size());
				return m_VulkanImageViewVector[idx];

			case ImageViewCreationMode::CreateForCube:
				FBY_ASSERT(false, "Not Implemented Yet!");
				return nullptr;

			case ImageViewCreationMode::DontCreate:
				FBY_ASSERT(0, "Cannot access image view at index: {} - Image was created with ImageViewCreationMode::DontCreate mode.", idx);
				return nullptr;
		}
	}

	Image::Image(const ImageSpecification& specification)
		: m_Specification(specification)
	{
		Invalidate();
	}

	Image::~Image()
	{
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		// Destroy Image Views
		switch (m_Specification.ViewCreationMode)
		{
			case ImageViewCreationMode::CreateUsingGivenSpecification:
				vkDestroyImageView(device, m_VulkanImageView, nullptr);
				break;
			case ImageViewCreationMode::CreateOnePerMipMap:
				for (VkImageView view : m_VulkanImageViewVector)
					vkDestroyImageView(device, view, nullptr);
				break;
			case ImageViewCreationMode::CreateForCube:
				FBY_ASSERT(false, "Not Implemented Yet!");
				break;
			case ImageViewCreationMode::DontCreate:
				break;
		}

		vkDestroyImage(device, m_VulkanImage, nullptr);
		vkFreeMemory(device, m_VkImageDeviceMemory, nullptr);
	}

	void Image::Invalidate()
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

		VK_CHECK_RESULT(vkCreateImage(device, &vk_image_create_info, nullptr, &m_VulkanImage));

		vkGetImageMemoryRequirements(device, m_VulkanImage, &m_MemoryRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = m_MemoryRequirements.size;
		allocInfo.memoryTypeIndex = RenderCommand::GetValidMemoryTypeIndex(physicalDevice, m_MemoryRequirements.memoryTypeBits, m_Specification.MemoryProperties);

		VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &m_VkImageDeviceMemory));
		vkBindImageMemory(device, m_VulkanImage, m_VkImageDeviceMemory, 0);

		switch (m_Specification.ViewCreationMode)
		{
			case ImageViewCreationMode::CreateUsingGivenSpecification:
			{
				VkImageViewCreateInfo vulkanImageViewCreateInfo{};
				vulkanImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				vulkanImageViewCreateInfo.image = m_VulkanImage;

				// This line is really weird
				vulkanImageViewCreateInfo.viewType = m_Specification.ViewSpecification.ViewType == VK_IMAGE_VIEW_TYPE_2D && m_Specification.ViewSpecification.LayerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : m_Specification.ViewSpecification.ViewType;

				vulkanImageViewCreateInfo.format = m_Specification.Format;
				vulkanImageViewCreateInfo.subresourceRange.aspectMask = m_Specification.ViewSpecification.AspectFlags;
				vulkanImageViewCreateInfo.subresourceRange.baseMipLevel = m_Specification.ViewSpecification.BaseMipLevel;
				vulkanImageViewCreateInfo.subresourceRange.levelCount = m_Specification.ViewSpecification.LevelCount;
				vulkanImageViewCreateInfo.subresourceRange.baseArrayLayer = m_Specification.ViewSpecification.BaseArrayLayer;
				vulkanImageViewCreateInfo.subresourceRange.layerCount = m_Specification.ViewSpecification.LayerCount;
				vulkanImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				vulkanImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				vulkanImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				vulkanImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

				VK_CHECK_RESULT(vkCreateImageView(device, &vulkanImageViewCreateInfo, nullptr, &m_VulkanImageView));
				break;
			}
			case ImageViewCreationMode::CreateOnePerMipMap:
			{
				m_VulkanImageViewVector.resize(m_Specification.MipLevels);

				for (int i = 0; i < m_Specification.MipLevels; i++)
				{
					VkImageViewCreateInfo vulkanImageViewCreateInfo{};

					vulkanImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					vulkanImageViewCreateInfo.image = m_VulkanImage;

					// This line is really weird
					vulkanImageViewCreateInfo.viewType = m_Specification.ViewSpecification.ViewType == VK_IMAGE_VIEW_TYPE_2D && m_Specification.ViewSpecification.LayerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : m_Specification.ViewSpecification.ViewType;

					vulkanImageViewCreateInfo.format = m_Specification.Format;
					vulkanImageViewCreateInfo.subresourceRange.aspectMask = m_Specification.ViewSpecification.AspectFlags;
					vulkanImageViewCreateInfo.subresourceRange.baseMipLevel = i;
					vulkanImageViewCreateInfo.subresourceRange.levelCount = 1;
					vulkanImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
					vulkanImageViewCreateInfo.subresourceRange.layerCount = 1;
					vulkanImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
					vulkanImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
					vulkanImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
					vulkanImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

					VK_CHECK_RESULT(vkCreateImageView(device, &vulkanImageViewCreateInfo, nullptr, &m_VulkanImageViewVector[i]));
				}
				break;
			}
			case ImageViewCreationMode::CreateForCube:
				FBY_ASSERT(false, "Not Implemented Yet!");
				break;
			case ImageViewCreationMode::DontCreate:
				break;
		}
	}

	void Image::OnResize(uint32_t width, uint32_t height, uint32_t mipLevels)
	{
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		// Destroy Image Views
		switch (m_Specification.ViewCreationMode)
		{
			case ImageViewCreationMode::CreateUsingGivenSpecification:
				vkDestroyImageView(device, m_VulkanImageView, nullptr);
				break;
			case ImageViewCreationMode::CreateOnePerMipMap:
				for (VkImageView view : m_VulkanImageViewVector)
					vkDestroyImageView(device, view, nullptr);
				break;
			case ImageViewCreationMode::CreateForCube:
				FBY_ASSERT(false, "Not Implemented Yet!");
				break;
			case ImageViewCreationMode::DontCreate:
				break;
		}

		vkDestroyImage(device, m_VulkanImage, nullptr);
		vkFreeMemory(device, m_VkImageDeviceMemory, nullptr);

		m_Specification.Width = width;
		m_Specification.Height = height;
		if (mipLevels)
			m_Specification.MipLevels = mipLevels;

		Invalidate();
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
		barrier.image = m_VulkanImage;
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
				m_VulkanImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				m_VulkanImage, oldLayout,
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

		vkCmdCopyBufferToImage(commandBuffer, srcBuffer, m_VulkanImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_buffer_image_copy_region);
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
		imageMemoryBarrier.image = m_VulkanImage;
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

	namespace Utils {

		VkImageView CreateImageViewUsingSpecification(const VkImage vulkanImage, const VkFormat format, const ImageViewSpecification& viewSpecification)
		{
			VkImageView vulkanImageView;

			VkImageViewCreateInfo vulkanImageViewCreateInfo{};
			vulkanImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			vulkanImageViewCreateInfo.image = vulkanImage;

			// This line is really weird
			vulkanImageViewCreateInfo.viewType =
				viewSpecification.ViewType == VK_IMAGE_VIEW_TYPE_2D && viewSpecification.LayerCount > 1
				? VK_IMAGE_VIEW_TYPE_2D_ARRAY
				: viewSpecification.ViewType;

			vulkanImageViewCreateInfo.format = format;
			vulkanImageViewCreateInfo.subresourceRange.aspectMask = viewSpecification.AspectFlags;
			vulkanImageViewCreateInfo.subresourceRange.baseMipLevel = viewSpecification.BaseMipLevel;
			vulkanImageViewCreateInfo.subresourceRange.levelCount = viewSpecification.LevelCount;
			vulkanImageViewCreateInfo.subresourceRange.baseArrayLayer = viewSpecification.BaseArrayLayer;
			vulkanImageViewCreateInfo.subresourceRange.layerCount = viewSpecification.LayerCount;
			vulkanImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			vulkanImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			vulkanImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			vulkanImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			VK_CHECK_RESULT(vkCreateImageView(device, &vulkanImageViewCreateInfo, nullptr, &vulkanImageView));

			return vulkanImageView;
		}

	} // namespace Utils

} // namespace Flameberry
