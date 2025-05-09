#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace Flameberry {

	enum class ImageViewCreationMode : uint8_t
	{
		// Ignores `ImageSpecification.ViewSpecification`
		DontCreate = 0,

		// Creates an image view as per the provided `ImageSpecification.ViewSpecification`
		CreateUsingGivenSpecification,

		// Creates a total of `ImageSpecification.MipLevels` image views, one per level
		// Note: This option overrides the `BaseMipLevel`, `LevelCount`, `BaseArrayLayer` and `LayerCount` options
		// ...provided in `ImageViewSpecification.ViewSpecification`
		CreateOnePerMipMap,

		// Todo: Implement this
		CreateForCube
	};

	struct ImageViewSpecification
	{
		VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageViewType ViewType = VK_IMAGE_VIEW_TYPE_2D;
		uint32_t BaseMipLevel = 0, LevelCount = 1, BaseArrayLayer = 0, LayerCount = 1;
	};

	struct ImageSpecification
	{
		uint32_t Width, Height;
		uint32_t Samples = 1, MipLevels = 1, ArrayLayers = 1;
		VkFormat Format;
		VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;
		VkImageUsageFlags Usage;
		VkMemoryPropertyFlags MemoryProperties;
		VkImageCreateFlags Flags = 0;

		// By observation, the codebase mostly uses `CreateUsingGivenSpecification` mode, hence this is the default
		ImageViewCreationMode ViewCreationMode = ImageViewCreationMode::CreateUsingGivenSpecification;

		ImageViewSpecification ViewSpecification;
	};

	class Image
	{
	public:
		Image(const ImageSpecification& specification);
		~Image();

		void GenerateMipmaps(VkImageLayout oldLayout, VkImageLayout newLayout);
		void CmdGenerateMipmaps(VkCommandBuffer cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);

		void WriteFromBuffer(VkBuffer srcBuffer);
		void OnResize(uint32_t width, uint32_t height, uint32_t mipLevels = 0);

		// This function just straight up creates, begins and ends a command buffer, which might be inefficient
		void TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
		void CmdTransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

		VkImage GetVulkanImage() const { return m_VulkanImage; }
		VkImageView GetVulkanImageView(int idx = 0) const;
		ImageSpecification GetSpecification() const { return m_Specification; }
		VkMemoryRequirements GetMemoryRequirements() const { return m_MemoryRequirements; }

	private:
		void Invalidate();

	private:
		VkImage m_VulkanImage;
		VkDeviceMemory m_VkImageDeviceMemory;

		VkMemoryRequirements m_MemoryRequirements;
		ImageSpecification m_Specification;

		// (Aditya): So I've made this decision choice to have a separate member for storing mipmap views
		// and a separate member for a single view. This is redundant but more convenient to use (hopefully).

		// This is gonna be used in case of the `CreateUsingGivenSpecification` mode
		VkImageView m_VulkanImageView;

		// This is gonna be used in case of the `CreateOnePerMipMap` mode
		std::vector<VkImageView> m_VulkanImageViewVector;
	};

	namespace Utils {

		VkImageView CreateImageViewUsingSpecification(const VkImage vulkanImage, const VkFormat format, const ImageViewSpecification& viewSpecification);

	}

} // namespace Flameberry
