#pragma once

#include <vector>

#include "VulkanVertex.h"
#include "StaticMesh.h"

namespace Flameberry {

	struct QueueFamilyIndices;

	struct SwapChainDetails
	{
		VkSurfaceCapabilitiesKHR SurfaceCapabilities;
		std::vector<VkSurfaceFormatKHR> SurfaceFormats;
		std::vector<VkPresentModeKHR> PresentationModes;
	};

	class RenderCommand
	{
	public:
		static bool DoesFormatSupportDepthAttachment(VkFormat format);
		static void WritePixelFromImageToBuffer(VkBuffer buffer, VkImage image, VkImageLayout currentImageLayout, const glm::vec2& pixelOffset);
		static void SetViewport(float x, float y, float width, float height);
		static void SetScissor(VkOffset2D offset, VkExtent2D extent);
		static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);
		static VkShaderModule CreateShaderModule(const std::vector<char>& compiledShaderCode);
		static VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice physicalDevice);
		static uint32_t GetValidMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags vk_memory_property_flags);
		static bool HasStencilComponent(VkFormat format);
		static VkFormat GetSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidateFormats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
		static bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers);
		static QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
		static SwapChainDetails GetSwapChainDetails(VkPhysicalDevice vk_device, VkSurfaceKHR surface);
		static std::vector<char> LoadCompiledShaderCode(const std::string& filePath);
		static VkSampler CreateDefaultSampler();
	};

} // namespace Flameberry
