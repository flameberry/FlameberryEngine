#include "Image.h"

#include <unordered_set>
#include <vector>

#include "vulkan/vulkan_core.h"

#include "Core/Assert.h"
#include "VulkanDebug.h"
#include "RenderCommand.h"
#include "VulkanContext.h"

namespace Flameberry {

	namespace Utils {

		VkImageAspectFlags GetAspectFlagsFromImageFormat(VkFormat format) {
			switch (format) {
				case VK_FORMAT_UNDEFINED:
					return 0;
				case VK_FORMAT_S8_UINT:
					return VK_IMAGE_ASPECT_STENCIL_BIT;
				case VK_FORMAT_D16_UNORM:
				case VK_FORMAT_X8_D24_UNORM_PACK32:
				case VK_FORMAT_D32_SFLOAT:
				case VK_FORMAT_D16_UNORM_S8_UINT:
				case VK_FORMAT_D24_UNORM_S8_UINT:
				case VK_FORMAT_D32_SFLOAT_S8_UINT:
					return VK_IMAGE_ASPECT_DEPTH_BIT;
				default:
					return VK_IMAGE_ASPECT_COLOR_BIT;
			}
		}

	}

	Image::Image(const ImageSpecification& specification)
		: m_Specification(specification), m_ActiveImageLayout(VK_IMAGE_LAYOUT_UNDEFINED) {
		Invalidate();
	}

	Image::~Image() {
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		DestroyImageViews();
		vkDestroyImage(device, m_VulkanImage, nullptr);
		vkFreeMemory(device, m_VkImageDeviceMemory, nullptr);
	}

	void Image::Invalidate() {
		CreateImage();
		AllocateAndBindMemory();
		CreateImageViews();
	}

	void Image::CreateImage() {
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = m_Specification.Width;
		imageCreateInfo.extent.height = m_Specification.Height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = m_Specification.MipLevels;
		imageCreateInfo.arrayLayers = m_Specification.ArrayLayers;
		imageCreateInfo.format = m_Specification.Format;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = m_Specification.Usage;
		imageCreateInfo.samples = (VkSampleCountFlagBits)m_Specification.Samples;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.flags = m_Specification.Flags;

		std::vector<VkFormat> formats = { m_Specification.Format };
		std::unordered_set<VkFormat> uniqueFormats = { m_Specification.Format };
		for (auto& view : m_Specification.Views) {
			if (view.Format != VK_FORMAT_UNDEFINED && view.Format != m_Specification.Format) {
				imageCreateInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
				if (uniqueFormats.find(view.Format) == uniqueFormats.end()) {
					uniqueFormats.insert(view.Format);
					formats.emplace_back(view.Format);
				}
			}
		}

		VkImageFormatListCreateInfo imageFormatListCreateInfo{};
		if (formats.size() > 1) {
			imageFormatListCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
			imageFormatListCreateInfo.pViewFormats = formats.data();
			imageFormatListCreateInfo.viewFormatCount = formats.size();

			imageCreateInfo.pNext = &imageFormatListCreateInfo;
		}

		VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &m_VulkanImage));
	}

	void Image::AllocateAndBindMemory() {
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		const auto& physicalDevice = VulkanContext::GetPhysicalDevice();

		vkGetImageMemoryRequirements(device, m_VulkanImage, &m_MemoryRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = m_MemoryRequirements.size;
		allocInfo.memoryTypeIndex = RenderCommand::GetValidMemoryTypeIndex(
			physicalDevice, m_MemoryRequirements.memoryTypeBits, m_Specification.MemoryProperties
		);

		VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &m_VkImageDeviceMemory));
		vkBindImageMemory(device, m_VulkanImage, m_VkImageDeviceMemory, 0);
	}

	void Image::CreateImageViews() {
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = m_VulkanImage;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		if (m_Specification.Views.empty()) m_Specification.Views.emplace_back();

		for (auto& view : m_Specification.Views) {
			FBY_ASSERT(
				view.BaseMipLevel < m_Specification.MipLevels, "Image view base mip level is out of the image's mip range"
			);
			FBY_ASSERT(
				view.BaseArrayLayer < m_Specification.ArrayLayers,
				"Image view base array layer is out of the image's layer range"
			);

			const uint32_t levelCount =
				view.LevelCount == VK_REMAINING_MIP_LEVELS ? m_Specification.MipLevels - view.BaseMipLevel : view.LevelCount;
			const uint32_t layerCount = view.LayerCount == VK_REMAINING_ARRAY_LAYERS
				? m_Specification.ArrayLayers - view.BaseArrayLayer
				: view.LayerCount;

			FBY_ASSERT(
				view.BaseMipLevel + levelCount <= m_Specification.MipLevels,
				"Image view mip range goes past the image's mip levels"
			);
			FBY_ASSERT(
				view.BaseArrayLayer + layerCount <= m_Specification.ArrayLayers,
				"Image view layer range goes past the image's array layers"
			);

			if (view.Type == VK_IMAGE_VIEW_TYPE_CUBE) {
				FBY_ASSERT(
					layerCount == 6, "An image view of type VK_IMAGE_VIEW_TYPE_CUBE must cover exactly 6 array layers"
				);
				FBY_ASSERT(
					m_Specification.Flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
					"An image view of type VK_IMAGE_VIEW_TYPE_CUBE must have the flag VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT set"
				);
			} else if (view.Type == VK_IMAGE_VIEW_TYPE_2D) {
				FBY_ASSERT(layerCount == 1, "An image view of type VK_IMAGE_VIEW_TYPE_2D must cover exactly 1 array layer");
			}

			// if view.Format is not provided, we assume it's the same as the image format
			view.Format = view.Format == VK_FORMAT_UNDEFINED ? m_Specification.Format : view.Format;

			imageViewCreateInfo.viewType = view.Type;
			imageViewCreateInfo.format = view.Format;
			imageViewCreateInfo.subresourceRange.aspectMask =
				view.AspectFlags ? view.AspectFlags : Utils::GetAspectFlagsFromImageFormat(view.Format);

			imageViewCreateInfo.subresourceRange.baseArrayLayer = view.BaseArrayLayer;
			imageViewCreateInfo.subresourceRange.layerCount = layerCount;

			if (view.OnePerMipLevel) {
				m_Views.emplace_back(levelCount, VK_NULL_HANDLE);

				for (uint32_t i = 0; i < levelCount; i++) {
					imageViewCreateInfo.subresourceRange.baseMipLevel = view.BaseMipLevel + i;
					imageViewCreateInfo.subresourceRange.levelCount = 1;

					VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_Views.back()[i]));
				}
			} else {
				m_Views.emplace_back(1, VK_NULL_HANDLE);

				imageViewCreateInfo.subresourceRange.baseMipLevel = view.BaseMipLevel;
				imageViewCreateInfo.subresourceRange.levelCount = levelCount;

				VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_Views.back()[0]));
			}
		}
	}

	void Image::OnResize(uint32_t width, uint32_t height, uint32_t mipLevels) {
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		DestroyImageViews();

		vkDestroyImage(device, m_VulkanImage, nullptr);
		vkFreeMemory(device, m_VkImageDeviceMemory, nullptr);

		m_Specification.Width = width;
		m_Specification.Height = height;
		if (mipLevels) m_Specification.MipLevels = mipLevels;
		m_ActiveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		Invalidate();
	}

	void Image::GenerateMipmaps(VkImageLayout oldLayout, VkImageLayout newLayout) {
		const auto& device = VulkanContext::GetCurrentDevice();
		VkCommandBuffer cmdBuffer;
		device->BeginSingleTimeCommandBuffer(cmdBuffer);
		CmdGenerateMipmaps(cmdBuffer, oldLayout, newLayout);
		device->EndSingleTimeCommandBuffer(cmdBuffer);
	}

	void Image::CmdGenerateMipmaps(VkCommandBuffer cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(VulkanContext::GetPhysicalDevice(), m_Specification.Format, &formatProperties);
		FBY_ASSERT(
			formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT,
			"Texture Image Format does not support linear blitting!"
		);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_VulkanImage;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = m_Specification.ArrayLayers;
		barrier.subresourceRange.levelCount = 1;

		if (m_Specification.MipLevels > 1) {
			barrier.subresourceRange.baseMipLevel = 1;
			barrier.subresourceRange.levelCount = m_Specification.MipLevels - 1;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(
				cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
				&barrier
			);

			barrier.subresourceRange.levelCount = 1;
		}

		int32_t mipWidth = m_Specification.Width;
		int32_t mipHeight = m_Specification.Height;

		for (uint32_t i = 1; i < m_Specification.MipLevels; i++) {
			const bool isBaseMip = i == 1;

			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = isBaseMip ? oldLayout : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask =
				isBaseMip ? VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT : VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(
				cmdBuffer, isBaseMip ? VK_PIPELINE_STAGE_ALL_COMMANDS_BIT : VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier
			);

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

			vkCmdBlitImage(
				cmdBuffer, m_VulkanImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_VulkanImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR
			);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = newLayout;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
				&barrier
			);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		const bool hasGeneratedMips = m_Specification.MipLevels > 1;

		barrier.subresourceRange.baseMipLevel = m_Specification.MipLevels - 1;
		barrier.oldLayout = hasGeneratedMips ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcAccessMask =
			hasGeneratedMips ? VK_ACCESS_TRANSFER_WRITE_BIT : VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			cmdBuffer, hasGeneratedMips ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
			&barrier
		);
	}

	void Image::WriteFromBuffer(VkBuffer srcBuffer) {
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

		vkCmdCopyBufferToImage(
			commandBuffer, srcBuffer, m_VulkanImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_buffer_image_copy_region
		);
		device->EndSingleTimeCommandBuffer(commandBuffer);
	}

	void Image::TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask) {
		const auto& device = VulkanContext::GetCurrentDevice();
		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

		device->BeginSingleTimeCommandBuffer(cmdBuffer);
		CmdTransitionLayout(cmdBuffer, oldLayout, newLayout, aspectMask);
		device->EndSingleTimeCommandBuffer(cmdBuffer);
	}

	void Image::CmdTransitionLayout(
		VkCommandBuffer cmdBuffer,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkImageAspectFlags aspectMask
	) {
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
		switch (oldLayout) {
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

			case VK_IMAGE_LAYOUT_GENERAL:
				// Image is used as a storage image
				// Make sure any shader writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newLayout) {
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
				imageMemoryBarrier.dstAccessMask =
					imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_GENERAL:
				// Image will be used as a storage image
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				break;
			default:
				break;
		}

		// Host accesses are not part of VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, so the host stage has to be added
		// explicitly
		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		if (imageMemoryBarrier.srcAccessMask & VK_ACCESS_HOST_WRITE_BIT) srcStageMask |= VK_PIPELINE_STAGE_HOST_BIT;

		// Put barrier inside setup command buffer
		vkCmdPipelineBarrier(
			cmdBuffer, srcStageMask, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
		);

		m_ActiveImageLayout = newLayout;
	}

	void Image::DestroyImageViews() {
		const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		for (auto& vv : m_Views) {
			for (VkImageView view : vv) vkDestroyImageView(device, view, nullptr);
		}
		m_Views.clear();
	}

	namespace Utils {

		VkImageView CreateImageViewUsingSpecification(
			const VkImage vulkanImage,
			const VkFormat format,
			const ImageViewSpecification& viewSpecification
		) {
			VkImageView vulkanImageView;

			VkImageViewCreateInfo vulkanImageViewCreateInfo{};
			vulkanImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			vulkanImageViewCreateInfo.image = vulkanImage;

			// This line is really weird
			vulkanImageViewCreateInfo.viewType =
				viewSpecification.Type == VK_IMAGE_VIEW_TYPE_2D && viewSpecification.LayerCount > 1
				? VK_IMAGE_VIEW_TYPE_2D_ARRAY
				: viewSpecification.Type;

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

	}

}
