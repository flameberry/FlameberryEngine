#include "Skymap.h"

#include <stb_image/stb_image.h>

#include "Renderer/VulkanContext.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/Buffer.h"
#include "Renderer/Pipeline.h"
#include "Renderer/Texture2D.h"

namespace Flameberry {

	Skymap::Skymap(const std::filesystem::path& filepath)
	{
		int width, height, channels, bytesPerChannel;
		void* pixels = nullptr;

		if (!stbi_is_hdr(filepath.c_str()))
			FBY_ERROR("Skymap cannot be made using a non-hdr file: {}", filepath);

		pixels = stbi_loadf(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		FBY_ASSERT(pixels, "Failed to load skymap from (Pixels are empty): {}", filepath);

		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
		bytesPerChannel = 4;
		const float imageSize = 4 * width * height * bytesPerChannel;

		Ref<Image> equirectangularImage;
		{
			ImageSpecification imageSpec;
			imageSpec.Width = width;
			imageSpec.Height = height;
			imageSpec.Format = format;
			imageSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			imageSpec.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageSpec.MipLevels = 1;

			equirectangularImage = CreateRef<Image>(imageSpec);

			BufferSpecification stagingBufferSpec;
			stagingBufferSpec.InstanceCount = 1;
			stagingBufferSpec.InstanceSize = imageSize;
			stagingBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			Buffer stagingBuffer(stagingBufferSpec);

			stagingBuffer.MapMemory(imageSize);
			stagingBuffer.WriteToBuffer(pixels, imageSize, 0);
			stagingBuffer.UnmapMemory();

			equirectangularImage->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			equirectangularImage->WriteFromBuffer(stagingBuffer.GetVulkanBuffer());
			equirectangularImage->TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		{
			ImageSpecification imageSpec;
			imageSpec.Width = width / 4;  // Questionable
			imageSpec.Height = width / 4; // Questionable
			imageSpec.Format = VK_FORMAT_R32G32B32A32_SFLOAT;
			imageSpec.ArrayLayers = 6;
			imageSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			imageSpec.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageSpec.Flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			imageSpec.ViewSpecification.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			imageSpec.ViewSpecification.ViewType = VK_IMAGE_VIEW_TYPE_CUBE;
			imageSpec.ViewSpecification.LayerCount = 6;

			// TODO: Figure out mip levels

			m_CubemapImage = CreateRef<Image>(imageSpec);
			m_CubemapImage->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		}

		stbi_image_free(pixels);

		// Pipeline
		ComputePipelineSpecification pipelineSpec;
		// TODO: Should this be moved into ShaderLibrary?
		pipelineSpec.Shader = CreateRef<Shader>(FBY_PROJECT_DIR "Flameberry/Shaders/Vulkan/Compiled/Flameberry_GenCubemap.comp.spv");

		Ref<ComputePipeline> cubemapGenerationPipeline = CreateRef<ComputePipeline>(pipelineSpec);

		DescriptorSetSpecification descSetSpec;
		// Get the DescriptorSetLayout for the Set of Index: 0 from the pipeline
		descSetSpec.Layout = cubemapGenerationPipeline->GetDescriptorSetLayout(0);

		Ref<DescriptorSet> descSet = CreateRef<DescriptorSet>(descSetSpec);

		VkDescriptorImageInfo equirectangularImageInfo{};
		equirectangularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		equirectangularImageInfo.imageView = equirectangularImage->GetImageView();
		equirectangularImageInfo.sampler = Texture2D::GetDefaultSampler();

		VkDescriptorImageInfo cubemapImageInfo{};
		cubemapImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		cubemapImageInfo.imageView = m_CubemapImage->GetImageView();
		cubemapImageInfo.sampler = VK_NULL_HANDLE;

		descSet->WriteImage(0, equirectangularImageInfo);
		descSet->WriteImage(1, cubemapImageInfo);
		descSet->Update();

		CommandBufferSpecification cmdBufferSpecification;
		cmdBufferSpecification.CommandPool = VulkanContext::GetCurrentDevice()->GetComputeCommandPool();
		cmdBufferSpecification.SingleTimeUsage = true;
		cmdBufferSpecification.IsPrimary = true;

		CommandBuffer cmdBuffer(cmdBufferSpecification);

		cmdBuffer.Begin();
		auto vulkanCmdBuffer = cmdBuffer.GetVulkanCommandBuffer();
		auto vulkanComputePipelineLayout = cubemapGenerationPipeline->GetVulkanPipelineLayout();
		auto vulkanComputePipeline = cubemapGenerationPipeline->GetVulkanPipeline();
		auto vulkanDescSet = descSet->GetVulkanDescriptorSet();

		vkCmdBindPipeline(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipeline);
		vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipelineLayout, 0, 1, &vulkanDescSet, 0, 0);

		vkCmdDispatch(vulkanCmdBuffer, (width / 4) / 8, (width / 4) / 8, 6);
		cmdBuffer.End();

		// Submit the command buffer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vulkanCmdBuffer;

		vkQueueSubmit(VulkanContext::GetCurrentDevice()->GetComputeQueue(), 1, &submitInfo, VK_NULL_HANDLE);

		// TODO: Remove this in the future
		VulkanContext::GetCurrentDevice()->WaitIdle();

		m_CubemapImage->TransitionLayout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorSetLayoutBinding descBinding{ .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT };

		DescriptorSetSpecification skymapDescSetSpec;
		skymapDescSetSpec.Layout = DescriptorSetLayout::CreateOrGetCached(DescriptorSetLayoutSpecification{ .Bindings = { descBinding } });
		;
		m_SkymapDescriptorSet = CreateRef<DescriptorSet>(skymapDescSetSpec);

		VkDescriptorImageInfo skymapImageInfo{};
		skymapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		skymapImageInfo.imageView = m_CubemapImage->GetImageView();
		skymapImageInfo.sampler = Texture2D::GetDefaultSampler();

		m_SkymapDescriptorSet->WriteImage(0, skymapImageInfo);
		m_SkymapDescriptorSet->Update();
	}

	Skymap::~Skymap()
	{
	}

} // namespace Flameberry
