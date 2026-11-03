#pragma once

#include "vulkan/vulkan_core.h"
#include <vector>
#include <vulkan/vulkan.h>

namespace Flameberry {

	struct ImageViewSpecification {
		VkFormat Format = VK_FORMAT_UNDEFINED;
		VkImageViewType Type = VK_IMAGE_VIEW_TYPE_2D;
		VkImageAspectFlags AspectFlags = 0;
		uint32_t BaseMipLevel = 0;
		uint32_t LevelCount = VK_REMAINING_MIP_LEVELS;
		uint32_t BaseArrayLayer = 0;
		uint32_t LayerCount = VK_REMAINING_ARRAY_LAYERS;
		bool OnePerMipLevel = false;
	};

	struct ImageSpecification {
		uint32_t Width;
		uint32_t Height;
		uint32_t Samples = 1;
		uint32_t MipLevels = 1;
		uint32_t ArrayLayers = 1;
		VkFormat Format;
		VkImageUsageFlags Usage;
		VkMemoryPropertyFlags MemoryProperties;
		VkImageCreateFlags Flags = 0;
		std::vector<ImageViewSpecification> Views;
	};

	class Image {
	public:
		Image(const ImageSpecification& specification);
		~Image();

		void GenerateMipmaps(VkImageLayout oldLayout, VkImageLayout newLayout);
		void CmdGenerateMipmaps(VkCommandBuffer cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);
		void WriteFromBuffer(VkBuffer srcBuffer);
		void OnResize(uint32_t width, uint32_t height, uint32_t mipLevels = 0);

		// This function just straight up creates, begins and ends a command buffer, which might be inefficient
		void TransitionLayout(
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
		);

		void CmdTransitionLayout(
			VkCommandBuffer cmdBuffer,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
		);

		VkImage GetVulkanImage() const { return m_VulkanImage; }
		VkImageView GetVulkanImageView(int idx = 0) const { return m_Views[idx][0]; }
		VkImageView GetVulkanImageMipView(int idx, int mip) const { return m_Views[idx][mip]; }
		VkImageLayout GetActiveImageLayout() const { return m_ActiveImageLayout; }
		const ImageSpecification& GetSpecification() const { return m_Specification; }

	private:
		void Invalidate();
		void CreateImage();
		void AllocateAndBindMemory();
		void CreateImageViews();
		void DestroyImageViews();

	private:
		VkImage m_VulkanImage;
		VkDeviceMemory m_VkImageDeviceMemory;
		VkMemoryRequirements m_MemoryRequirements;
		VkImageLayout m_ActiveImageLayout;
		ImageSpecification m_Specification;

		std::vector<std::vector<VkImageView>> m_Views;
	};

	namespace Utils {

		VkImageView CreateImageViewUsingSpecification(
			const VkImage vulkanImage,
			const VkFormat format,
			const ImageViewSpecification& viewSpecification
		);

	}

}
