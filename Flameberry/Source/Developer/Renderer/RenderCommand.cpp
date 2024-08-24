#include "RenderCommand.h"

#include <fstream>
#include <unordered_map>

#include "VulkanDebug.h"
#include "Core/Timer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"

#include "Renderer/Material.h"
#include "Renderer/Renderer.h"

namespace Flameberry {

	bool RenderCommand::DoesFormatSupportDepthAttachment(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT
			|| format == VK_FORMAT_D16_UNORM
			|| format == VK_FORMAT_D16_UNORM_S8_UINT
			|| format == VK_FORMAT_D24_UNORM_S8_UINT
			|| format == VK_FORMAT_D32_SFLOAT_S8_UINT;
	}

	void RenderCommand::WritePixelFromImageToBuffer(VkBuffer buffer, VkImage image, VkImageLayout currentImageLayout, const glm::vec2& pixelOffset)
	{
		const auto& device = VulkanContext::GetCurrentDevice();

		VkCommandBuffer commandBuffer;
		device->BeginSingleTimeCommandBuffer(commandBuffer);
		{
			VkPipelineStageFlags sourceStageFlags;
			VkPipelineStageFlags destinationStageFlags;

			VkImageMemoryBarrier vk_image_memory_barrier{};

			sourceStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vk_image_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			vk_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vk_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			vk_image_memory_barrier.oldLayout = currentImageLayout;
			vk_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			vk_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			vk_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			vk_image_memory_barrier.image = image;
			vk_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			vk_image_memory_barrier.subresourceRange.baseMipLevel = 0;
			vk_image_memory_barrier.subresourceRange.levelCount = 1;
			vk_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
			vk_image_memory_barrier.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(commandBuffer, sourceStageFlags, destinationStageFlags, 0, 0, nullptr, 0, nullptr, 1, &vk_image_memory_barrier);
		}

		VkBufferImageCopy region{};
		region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.imageOffset = { (int)pixelOffset.x, (int)pixelOffset.y };
		region.imageExtent = { 1, 1, 1 };

		region.bufferOffset = 0;
		region.bufferRowLength = 1;
		region.bufferImageHeight = 1;

		vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

		{
			VkPipelineStageFlags sourceStageFlags;
			VkPipelineStageFlags destinationStageFlags;

			VkImageMemoryBarrier vk_image_memory_barrier{};

			sourceStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vk_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vk_image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			vk_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			vk_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			vk_image_memory_barrier.newLayout = currentImageLayout;
			vk_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			vk_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			vk_image_memory_barrier.image = image;
			vk_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			vk_image_memory_barrier.subresourceRange.baseMipLevel = 0;
			vk_image_memory_barrier.subresourceRange.levelCount = 1;
			vk_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
			vk_image_memory_barrier.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(commandBuffer, sourceStageFlags, destinationStageFlags, 0, 0, nullptr, 0, nullptr, 1, &vk_image_memory_barrier);
		}
		device->EndSingleTimeCommandBuffer(commandBuffer);
	}

	void RenderCommand::SetViewport(float x, float y, float width, float height)
	{
		Renderer::Submit([x, y, width, height](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
			{
				VkViewport viewport{};
				viewport.x = x;
				viewport.y = y;
				viewport.width = width;
				viewport.height = height;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;

				vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			});
	}

	void RenderCommand::SetScissor(VkOffset2D offset, VkExtent2D extent)
	{
		Renderer::Submit([offset, extent](VkCommandBuffer cmdBuffer, uint32_t imageIndex)
			{
				VkRect2D scissor{};
				scissor.offset = offset;
				scissor.extent = extent;

				vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
			});
	}

	void RenderCommand::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
	{
		const auto& device = VulkanContext::GetCurrentDevice();

		VkCommandBuffer commandBuffer;
		device->BeginSingleTimeCommandBuffer(commandBuffer);
		VkBufferCopy vk_buffer_copy_info{};
		vk_buffer_copy_info.srcOffset = 0;
		vk_buffer_copy_info.dstOffset = 0;
		vk_buffer_copy_info.size = bufferSize;

		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &vk_buffer_copy_info);
		device->EndSingleTimeCommandBuffer(commandBuffer);
	}

	VkShaderModule RenderCommand::CreateShaderModule(const std::vector<char>& compiledShaderCode)
	{
		const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		VkShaderModuleCreateInfo vk_shader_module_create_info{};
		vk_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vk_shader_module_create_info.codeSize = compiledShaderCode.size();
		vk_shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(compiledShaderCode.data());

		VkShaderModule shaderModule;
		VK_CHECK_RESULT(vkCreateShaderModule(device, &vk_shader_module_create_info, nullptr, &shaderModule));
		return shaderModule;
	}

	VkSampleCountFlagBits RenderCommand::GetMaxUsableSampleCount(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		if (counts & VK_SAMPLE_COUNT_64_BIT)
		{
			return VK_SAMPLE_COUNT_64_BIT;
		}
		if (counts & VK_SAMPLE_COUNT_32_BIT)
		{
			return VK_SAMPLE_COUNT_32_BIT;
		}
		if (counts & VK_SAMPLE_COUNT_16_BIT)
		{
			return VK_SAMPLE_COUNT_16_BIT;
		}
		if (counts & VK_SAMPLE_COUNT_8_BIT)
		{
			return VK_SAMPLE_COUNT_8_BIT;
		}
		if (counts & VK_SAMPLE_COUNT_4_BIT)
		{
			return VK_SAMPLE_COUNT_4_BIT;
		}
		if (counts & VK_SAMPLE_COUNT_2_BIT)
		{
			return VK_SAMPLE_COUNT_2_BIT;
		}
		return VK_SAMPLE_COUNT_1_BIT;
	}

	uint32_t RenderCommand::GetValidMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags vk_memory_property_flags)
	{
		VkPhysicalDeviceMemoryProperties vk_physical_device_mem_properties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &vk_physical_device_mem_properties);

		for (uint32_t i = 0; i < vk_physical_device_mem_properties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (vk_physical_device_mem_properties.memoryTypes[i].propertyFlags & vk_memory_property_flags) == vk_memory_property_flags)
				return i;
		}
		FBY_ERROR("Failed to find valid memory type!");
		return -1;
	}

	bool RenderCommand::HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	VkFormat RenderCommand::GetSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidateFormats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
	{
		for (const auto& format : candidateFormats)
		{
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

			if ((tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
				|| (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags))
				return format;
		}
		FBY_ERROR("Couldn't find supported format!");
		return VK_FORMAT_UNDEFINED;
	}

	bool RenderCommand::CheckValidationLayerSupport(const std::vector<const char*>& validationLayers)
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> available_layers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, available_layers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;
			for (const auto& layerProperties : available_layers)
			{
				if (!strcmp(layerName, layerProperties.layerName))
				{
					layerFound = true;
					break;
				}
			}
			if (!layerFound)
			{
				FBY_ERROR("Failed to find the layer named '{}'!", layerName);
				return false;
			}
		}

		// Prints the names of the validation layers available
		std::string layer_list = "";
		for (uint16_t i = 0; i < layerCount; i++)
		{
			layer_list += available_layers[i].layerName;
			if (!(i == layerCount - 1))
				layer_list += ", ";
		}
		FBY_TRACE("Found the following Vulkan Validation Layers: {}", layer_list);
		return true;
	}

	QueueFamilyIndices RenderCommand::GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());

		uint32_t index = 0;
		QueueFamilyIndices indices;
		for (const auto& queue : queueFamilyProps)
		{
#if 0
			// TOOD: Check for separate compute device available
			if ((queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queue.queueFlags & VK_QUEUE_COMPUTE_BIT) && (queue.timestampValidBits != 0))
				indices.GraphicsAndComputeSupportedQueueFamilyIndex = index;
#else
			if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT && queue.timestampValidBits != 0)
				indices.GraphicsQueueFamilyIndex = index;

			if (queue.queueFlags & VK_QUEUE_COMPUTE_BIT && queue.timestampValidBits != 0)
				indices.ComputeQueueFamilyIndex = index;
#endif

			VkBool32 isPresentationSupported = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &isPresentationSupported);

			if (isPresentationSupported)
				indices.PresentationSupportedQueueFamilyIndex = index;

			if (indices.GraphicsQueueFamilyIndex != -1 && indices.ComputeQueueFamilyIndex != -1 && indices.PresentationSupportedQueueFamilyIndex != -1)
				break;

			index++;
		}
		return indices;
	}

	SwapChainDetails RenderCommand::GetSwapChainDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		SwapChainDetails vk_swap_chain_details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &vk_swap_chain_details.SurfaceCapabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

		if (formatCount)
		{
			vk_swap_chain_details.SurfaceFormats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, vk_swap_chain_details.SurfaceFormats.data());
		}

		uint32_t presentationModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationModeCount, nullptr);

		if (presentationModeCount)
		{
			vk_swap_chain_details.PresentationModes.resize(presentationModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationModeCount, vk_swap_chain_details.PresentationModes.data());
		}

		return vk_swap_chain_details;
	}

	VkSampler RenderCommand::CreateDefaultSampler()
	{
		VkSampler sampler;
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &sampler));
		return sampler;
	}

	std::vector<char> RenderCommand::LoadCompiledShaderCode(const std::string& filePath)
	{
		std::ifstream stream(filePath, std::ios::ate | std::ios::binary);
		if (!stream.is_open())
		{
			FBY_ERROR("Failed to load compiled shader code: {}", filePath);
			return {};
		}

		size_t fileSize = (size_t)stream.tellg();
		FBY_TRACE("File size of buffer taken from '{}' is {}", filePath, fileSize);

		std::vector<char> buffer(fileSize);
		stream.seekg(0);
		stream.read(buffer.data(), fileSize);
		stream.close();
		return buffer;
	}

} // namespace Flameberry
