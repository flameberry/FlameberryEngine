#include "Skymap.h"

#include <stb_image/stb_image.h>

#include "Core/Core.h"
#include "Core/Timer.h"

#include "Renderer/Shader.h"
#include "Renderer/ShaderLibrary.h"
#include "Renderer/VulkanContext.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/Buffer.h"
#include "Renderer/Pipeline.h"
#include "Renderer/Texture2D.h"
#include "Renderer/VulkanDebug.h"
#include "Renderer/Material.h"

namespace Flameberry {

	Ref<DescriptorSet> Skymap::s_EmptyDescriptorSet;
	Ref<Image> Skymap::s_EmptyCubemap;
	std::unordered_map<uint32_t, Ref<Image>> Skymap::s_CubemapSizeToBRDFLUTMap;

	Skymap::Skymap(const std::filesystem::path& filepath)
	{
		FBY_SCOPED_TIMER("Cubemap_IrradianceMap_Prefiltered_Gen");

		int width, height, channels, bytesPerChannel;
		void* pixels = nullptr;

		// Confirm that the skymap is an HDR file
		if (!stbi_is_hdr(filepath.c_str()))
			FBY_ERROR("Skymap cannot be made using a non-hdr file: {}", filepath);

		// Load the equirectangular map
		pixels = stbi_loadf(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		// Assert that the pixels are not empty
		FBY_ASSERT(pixels, "Failed to load skymap from (Pixels are empty): {}", filepath);

		// VK_FORMAT_R32G32B32A32_SFLOAT is the default hardcoded format we are considering
		VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
		// R32B32G32A32 -> 32 bits per channel -> 4 bytes per channel
		bytesPerChannel = 4;
		// Calculate the image size manually
		const float imageSize = 4 * width * height * bytesPerChannel;

		// This will store the flat equirectangular image which will only be used to create the necessary cubemaps for IBL
		Ref<Image> equirectangularImage;
		{
			// Create a Vulkan Image based on the pixel data we loaded using stbi_image
			ImageSpecification imageSpec;
			imageSpec.Width = width;
			imageSpec.Height = height;
			imageSpec.Format = format;
			imageSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			// Transfer destination for the stagingBuffer
			// Sampled by compute shader to convert it into a cubemap
			imageSpec.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageSpec.MipLevels = 1;

			// Creation of the Image
			equirectangularImage = CreateRef<Image>(imageSpec);

			// Create a stagingBuffer to transfer the pixel data from CPU to GPU
			BufferSpecification stagingBufferSpec;
			stagingBufferSpec.InstanceCount = 1;
			stagingBufferSpec.InstanceSize = imageSize; // Use the calculated imageSize here
			stagingBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			// Creation of the actual staging buffer
			Buffer stagingBuffer(stagingBufferSpec);

			// Transfer of the pixel data from CPU to the staging buffer
			stagingBuffer.MapMemory(imageSize);
			stagingBuffer.WriteToBuffer(pixels, imageSize, 0);
			stagingBuffer.UnmapMemory();

			// Before transferring it is important to ensure that the imageLayout is set to be the destination
			equirectangularImage->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			// Copy the pixel data from the staging buffer to the actual image in GPU
			equirectangularImage->WriteFromBuffer(stagingBuffer.GetVulkanBuffer());
			// After transferring it is important to ensure that the imageLayout is correct to make the image ready for use
			equirectangularImage->TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		// Calculate the number of mip levels based upon the image dimensions
		// Each mip will represent a roughness level that we will calculate the prefiltered map for
		const uint32_t mipLevels = static_cast<uint32_t>(floor(log2(width / 4))) + 1;
		// const uint32_t mipLevels = 8;

		// Creation of the destination cubemap where our main skymap will be stored
		{
			ImageSpecification imageSpec;
			imageSpec.Width = width / 4;  // Questionable
			imageSpec.Height = width / 4; // Questionable
			imageSpec.Format = VK_FORMAT_R32G32B32A32_SFLOAT;
			// 6 Layers for 6 faces
			imageSpec.ArrayLayers = 6;
			imageSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			// Storage Image for the compute shaders to store data from equirectangular map
			// Sampled by the PBR fragment shader for rendering
			imageSpec.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			// Important flag to ensure that we can sample the image as a cube
			imageSpec.Flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			imageSpec.ViewSpecification.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			// VkImageView type should be VK_IMAGE_VIEW_TYPE_CUBE
			imageSpec.ViewSpecification.ViewType = VK_IMAGE_VIEW_TYPE_CUBE;
			// 6 Layers for 6 faces
			imageSpec.ViewSpecification.LayerCount = 6;

			// TODO: Figure out mip levels
			imageSpec.MipLevels = mipLevels;

			// Creation of the cubemap image
			m_CubemapImage = CreateRef<Image>(imageSpec);
		}

		{
			// Creation of the Irradiance Map
			// This process is almost the same as the creation of the m_CubemapImage
			// Except the fact that the m_CubemapImage may have multiple mipmaps in the future
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
			imageSpec.MipLevels = 1;

			m_IrradianceMap = CreateRef<Image>(imageSpec);
		}

		// The image views for accessing each mip in the compute shader
		VkImageView prefilteredMipMapImageViews[mipLevels];
		{
			// Creation of the Prefiltered Map
			// This process is almost the same as the creation of the m_CubemapImage
			// Except the fact that the PrefilteredMap has multiple mipmaps
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

			// This is the distinguishing factor between prefiltered and the other image creation methods
			imageSpec.MipLevels = mipLevels;

			m_PrefilteredMap = CreateRef<Image>(imageSpec);

			// Creating image views for accessing each mip in the compute shader
			for (uint8_t i = 0; i < mipLevels; i++)
			{
				VkImageViewCreateInfo imageViewCreateInfo{};
				imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewCreateInfo.image = m_PrefilteredMap->GetVulkanImage();
				imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
				imageViewCreateInfo.format = imageSpec.Format;
				imageViewCreateInfo.subresourceRange.aspectMask = imageSpec.ViewSpecification.AspectFlags;

				// This is the important part, where the base mip level is the current mip level that we are iterating
				imageViewCreateInfo.subresourceRange.baseMipLevel = i;
				imageViewCreateInfo.subresourceRange.levelCount = 1;
				imageViewCreateInfo.subresourceRange.baseArrayLayer = imageSpec.ViewSpecification.BaseArrayLayer;
				imageViewCreateInfo.subresourceRange.layerCount = imageSpec.ViewSpecification.LayerCount;

				imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

				const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
				VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &prefilteredMipMapImageViews[i]));
			}

			// Create Sampler that enables sampling multiple LODs
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.anisotropyEnable = VK_TRUE;

			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(VulkanContext::GetPhysicalDevice(), &properties);

			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = static_cast<float>(m_PrefilteredMap->GetSpecification().MipLevels);

			VK_CHECK_RESULT(vkCreateSampler(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), &samplerInfo, nullptr, &m_MultiLODSampler));
		}

		const auto brdflutMap = s_CubemapSizeToBRDFLUTMap.find(width / 4);
		const bool shouldGenBRDFLUT = brdflutMap == s_CubemapSizeToBRDFLUTMap.end();

		if (shouldGenBRDFLUT)
		{
			ImageSpecification imageSpec;
			imageSpec.Width = width / 4;  // Questionable
			imageSpec.Height = width / 4; // Questionable
			imageSpec.Format = VK_FORMAT_R16G16_SFLOAT;
			imageSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			imageSpec.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageSpec.ViewSpecification.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

			m_BRDFLUTMap = CreateRef<Image>(imageSpec);

			s_CubemapSizeToBRDFLUTMap[width / 4] = m_BRDFLUTMap;
		}
		else
		{
			m_BRDFLUTMap = brdflutMap->second;
		}

		// Free the equirectangular map pixels loaded on the CPU
		stbi_image_free(pixels);

		// Cubemap Generation Pipeline ---------------------------------------------------------------------------------------------
		Ref<ComputePipeline> cubemapGenerationPipeline;
		Ref<DescriptorSet> cubemapGenerationDescriptorSet;
		{
			ComputePipelineSpecification pipelineSpec;
			// TODO: Should this be moved into ShaderLibrary?
			pipelineSpec.Shader = ShaderLibrary::Get("HDRToCubemap");
			cubemapGenerationPipeline = CreateRef<ComputePipeline>(pipelineSpec);

			// Cubemap Generation DescriptorSet
			DescriptorSetSpecification descSetSpec;
			// Get the DescriptorSetLayout for the Set of Index: 0 from the pipeline
			descSetSpec.Layout = cubemapGenerationPipeline->GetDescriptorSetLayout(0);
			// Create a descriptor set using that layout
			cubemapGenerationDescriptorSet = CreateRef<DescriptorSet>(descSetSpec);

			// First binding is the equirectangularImage which is a sampler2D
			VkDescriptorImageInfo equirectangularImageInfo{};
			equirectangularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			equirectangularImageInfo.imageView = equirectangularImage->GetVulkanImageView();
			equirectangularImageInfo.sampler = Texture2D::GetDefaultSampler();

			// Second binding is the destination m_CubemapImage which is a imageCube, which will be the target of the compute shader
			VkDescriptorImageInfo cubemapImageInfo{};
			cubemapImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			cubemapImageInfo.imageView = m_CubemapImage->GetVulkanImageView();
			cubemapImageInfo.sampler = VK_NULL_HANDLE;

			// Write both the bindings to their binding indices and update the descriptor set
			cubemapGenerationDescriptorSet->WriteImage(0, equirectangularImageInfo);
			cubemapGenerationDescriptorSet->WriteImage(1, cubemapImageInfo);
			cubemapGenerationDescriptorSet->Update();
		}

		// Creation of Irradiance Pipeline ---------------------------------------------------------------------------------------------
		Ref<ComputePipeline> irrandianceMapGenerationPipeline;
		Ref<DescriptorSet> irradianceMapGenerationDescriptorSet;
		{
			// Irradiance Map will be generated using the m_CubemapImage
			ComputePipelineSpecification irrPipelineSpec;
			irrPipelineSpec.Shader = ShaderLibrary::Get("IrradianceMap");
			irrandianceMapGenerationPipeline = CreateRef<ComputePipeline>(irrPipelineSpec);

			// Irradiance Descriptor Set
			DescriptorSetSpecification irrDescSetSpec;
			// Get the DescriptorSetLayout for the Set of Index: 0 from the pipeline
			irrDescSetSpec.Layout = irrandianceMapGenerationPipeline->GetDescriptorSetLayout(0);
			// Create a descriptor set using that layout
			irradianceMapGenerationDescriptorSet = CreateRef<DescriptorSet>(irrDescSetSpec);

			// First binding will be the m_CubemapImage to which we are gonna write to in the first step
			VkDescriptorImageInfo cubemapSamplerInfo{};
			cubemapSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			cubemapSamplerInfo.imageView = m_CubemapImage->GetVulkanImageView();
			cubemapSamplerInfo.sampler = m_MultiLODSampler;

			// Second binding will be the target irradiance map
			VkDescriptorImageInfo irradianceMapImageInfo{};
			irradianceMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			irradianceMapImageInfo.imageView = m_IrradianceMap->GetVulkanImageView();
			irradianceMapImageInfo.sampler = VK_NULL_HANDLE;

			// Write both the bindings to their binding indices and update the descriptor set
			irradianceMapGenerationDescriptorSet->WriteImage(0, cubemapSamplerInfo);
			irradianceMapGenerationDescriptorSet->WriteImage(1, irradianceMapImageInfo);
			irradianceMapGenerationDescriptorSet->Update();
		}

		// Creation of the PrefilteredMap generation pipeline ---------------------------------------------------------------------------
		Ref<ComputePipeline> prefilteredMapGenerationPipeline;
		Ref<Material> prefilteredMapMaterial;
		Ref<DescriptorSet> prefilteredMapGenerationDescriptorSets[mipLevels];
		VkDescriptorImageInfo prefilteredMapImageInfos[mipLevels];
		{
			ComputePipelineSpecification prefilteredPipelineSpec;
			prefilteredPipelineSpec.Shader = ShaderLibrary::Get("PrefilteredMap");
			prefilteredPipelineSpec.SpecializationConstantLayout = {
				{ 0, ShaderDataType::Int }
			};
			prefilteredPipelineSpec.SpecializationConstantData = &mipLevels;

			prefilteredMapGenerationPipeline = CreateRef<ComputePipeline>(prefilteredPipelineSpec);

			prefilteredMapMaterial = CreateRef<Material>(prefilteredMapGenerationPipeline->GetSpecification().Shader);

			for (uint8_t i = 0; i < mipLevels; i++)
			{
				// Prefiltered Map Descriptor Set
				DescriptorSetSpecification prefilteredDescSetSpec;
				// Get the DescriptorSetLayout for the Set of Index: 0 from the pipeline
				prefilteredDescSetSpec.Layout = prefilteredMapGenerationPipeline->GetDescriptorSetLayout(0);
				// Create a descriptor set using that layout
				prefilteredMapGenerationDescriptorSets[i] = CreateRef<DescriptorSet>(prefilteredDescSetSpec);

				// First binding will be the m_CubemapImage to which we are gonna write to in the first step
				VkDescriptorImageInfo cubemapSamplerInfo{};
				cubemapSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				cubemapSamplerInfo.imageView = m_CubemapImage->GetVulkanImageView();
				cubemapSamplerInfo.sampler = m_MultiLODSampler;

				// Write both the bindings to their binding indices and update the descriptor set
				prefilteredMapGenerationDescriptorSets[i]->WriteImage(0, cubemapSamplerInfo);

				prefilteredMapImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				prefilteredMapImageInfos[i].imageView = prefilteredMipMapImageViews[i];
				prefilteredMapImageInfos[i].sampler = VK_NULL_HANDLE;

				prefilteredMapGenerationDescriptorSets[i]->WriteImage(1, prefilteredMapImageInfos[i]);
				prefilteredMapGenerationDescriptorSets[i]->Update();
			}
		}

		// Creation of the BRDFLUT generation pipeline ---------------------------------------------------------------------------
		Ref<ComputePipeline> pipelineBRDFLUT;
		Ref<DescriptorSet> descSetBRDFLUT;

		if (shouldGenBRDFLUT)
		{
			ComputePipelineSpecification brdflutPipelineSpec;
			brdflutPipelineSpec.Shader = ShaderLibrary::Get("BRDFLUT");
			brdflutPipelineSpec.SpecializationConstantLayout = {
				{ 0, ShaderDataType::Int }
			};

			const int brdflutImageSize = width / 4;
			brdflutPipelineSpec.SpecializationConstantData = &brdflutImageSize;

			pipelineBRDFLUT = CreateRef<ComputePipeline>(brdflutPipelineSpec);

			// BRDFLUT Descriptor Set
			DescriptorSetSpecification brdflutDescSetSpec;
			// Get the DescriptorSetLayout for the Set of Index: 0 from the pipeline
			brdflutDescSetSpec.Layout = pipelineBRDFLUT->GetDescriptorSetLayout(0);
			// Create a descriptor set using that layout
			descSetBRDFLUT = CreateRef<DescriptorSet>(brdflutDescSetSpec);

			// First binding will be the m_BRDFLUTMap to which we are gonna write to in the first step
			VkDescriptorImageInfo brdflutMapImageInfo{};
			brdflutMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			brdflutMapImageInfo.imageView = m_BRDFLUTMap->GetVulkanImageView();
			brdflutMapImageInfo.sampler = VK_NULL_HANDLE;

			// Write the binding to it's binding index and update the descriptor set
			descSetBRDFLUT->WriteImage(0, brdflutMapImageInfo);
			descSetBRDFLUT->Update();
		}

		// The command buffer to run the whole process --------------------------------------------------------------------------------
		CommandBufferSpecification cmdBufferSpecification;
		cmdBufferSpecification.CommandPool = VulkanContext::GetCurrentDevice()->GetComputeCommandPool();
		cmdBufferSpecification.SingleTimeUsage = true;
		cmdBufferSpecification.IsPrimary = true;

		// Creation of the command buffer
		CommandBuffer cmdBuffer(cmdBufferSpecification);

		// Recording the Command Buffer
		cmdBuffer.Begin();

		// Step 1: Generation of the cube map using the equirectangular image
		auto vulkanCmdBuffer = cmdBuffer.GetVulkanCommandBuffer();
		auto vulkanComputePipelineLayout = cubemapGenerationPipeline->GetVulkanPipelineLayout();
		auto vulkanComputePipeline = cubemapGenerationPipeline->GetVulkanPipeline();
		auto vulkanDescSet = cubemapGenerationDescriptorSet->GetVulkanDescriptorSet();

		// First transition all the images into a layout that can be read from or written to
		m_CubemapImage->CmdTransitionLayout(vulkanCmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		m_IrradianceMap->CmdTransitionLayout(vulkanCmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		m_PrefilteredMap->CmdTransitionLayout(vulkanCmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		// Bind the pipeline, descriptor sets and dispatch the compute shader for the first step
		vkCmdBindPipeline(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipeline);
		vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipelineLayout, 0, 1, &vulkanDescSet, 0, 0);
		vkCmdDispatch(vulkanCmdBuffer, (width / 4) / 8, (width / 4) / 8, 6);

		// After we have written to the cube map, we make it read only
		m_CubemapImage->CmdGenerateMipmaps(vulkanCmdBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// Step 2: Generation of the irradiance map using the cubemap image
		vulkanComputePipelineLayout = irrandianceMapGenerationPipeline->GetVulkanPipelineLayout();
		vulkanComputePipeline = irrandianceMapGenerationPipeline->GetVulkanPipeline();
		vulkanDescSet = irradianceMapGenerationDescriptorSet->GetVulkanDescriptorSet();

		// Bind the pipeline, descriptor sets and dispatch the compute shader for the second step
		vkCmdBindPipeline(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipeline);
		vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipelineLayout, 0, 1, &vulkanDescSet, 0, 0);
		vkCmdDispatch(vulkanCmdBuffer, (width / 4) / 8, (width / 4) / 8, 6);

		// After we have written to the irradiance map, we make it read only
		m_IrradianceMap->CmdTransitionLayout(vulkanCmdBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// Step 3: Generation of the prefiltered map using the cubemap image
		vulkanComputePipelineLayout = prefilteredMapGenerationPipeline->GetVulkanPipelineLayout();
		vulkanComputePipeline = prefilteredMapGenerationPipeline->GetVulkanPipeline();

		vkCmdBindPipeline(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipeline);

		uint32_t workGroupSize = width / 4;
		for (uint8_t i = 0; i < mipLevels; i++)
		{
			// Push Constants
			prefilteredMapMaterial->Set("u_Data.Resolution", (float)width / 4.0f);
			prefilteredMapMaterial->Set("u_Data.MipLevel", (float)i);
			vkCmdPushConstants(vulkanCmdBuffer, vulkanComputePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, prefilteredMapMaterial->GetPushConstantOffset(), prefilteredMapMaterial->GetUniformDataSize(), prefilteredMapMaterial->GetUniformDataPtr());

			// Bind the descriptor sets and dispatch the compute shader for the third step
			vulkanDescSet = prefilteredMapGenerationDescriptorSets[i]->GetVulkanDescriptorSet();
			vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipelineLayout, 0, 1, &vulkanDescSet, 0, 0);
			vkCmdDispatch(vulkanCmdBuffer, workGroupSize / 8, workGroupSize / 8, 6);

			workGroupSize = glm::max<uint32_t>(workGroupSize / 2, 8);
		}

		// After we have written to the prefiltered map, we make it read only
		m_PrefilteredMap->CmdTransitionLayout(vulkanCmdBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		if (shouldGenBRDFLUT)
		{
			m_BRDFLUTMap->CmdTransitionLayout(vulkanCmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

			// Step 4: Generation of the BRDFLUT map
			vulkanComputePipelineLayout = pipelineBRDFLUT->GetVulkanPipelineLayout();
			vulkanComputePipeline = pipelineBRDFLUT->GetVulkanPipeline();
			vulkanDescSet = descSetBRDFLUT->GetVulkanDescriptorSet();

			// Bind the pipeline and dispatch the compute shader for the fourth step
			vkCmdBindPipeline(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipeline);
			vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanComputePipelineLayout, 0, 1, &vulkanDescSet, 0, 0);
			vkCmdDispatch(vulkanCmdBuffer, (width / 4) / 8, (width / 4) / 8, 1);

			m_BRDFLUTMap->CmdTransitionLayout(vulkanCmdBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		// End the command buffer recording
		cmdBuffer.End();

		// Submit the command buffer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &vulkanCmdBuffer;

		VK_CHECK_RESULT(vkQueueSubmit(VulkanContext::GetCurrentDevice()->GetComputeQueue(), 1, &submitInfo, VK_NULL_HANDLE));

		// Now that all the processing is done, we need to bring all the skymap resources into a descriptor set to be used later by the PBR pipeline
		std::vector<VkDescriptorSetLayoutBinding> descBindings = {
			{ .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
			{ .binding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
			{ .binding = 2, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
			{ .binding = 3, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT }
		};

		DescriptorSetSpecification skymapDescSetSpec;
		skymapDescSetSpec.Layout = DescriptorSetLayout::CreateOrGetCached(DescriptorSetLayoutSpecification{ .Bindings = descBindings });
		m_SkymapDescriptorSet = CreateRef<DescriptorSet>(skymapDescSetSpec);

		VkDescriptorImageInfo skymapImageInfo{};
		skymapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		skymapImageInfo.imageView = m_CubemapImage->GetVulkanImageView();
		skymapImageInfo.sampler = m_MultiLODSampler;

		VkDescriptorImageInfo irrImageInfo{};
		irrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		irrImageInfo.imageView = m_IrradianceMap->GetVulkanImageView();
		irrImageInfo.sampler = Texture2D::GetDefaultSampler();

		VkDescriptorImageInfo prefilteredMapImageInfo{};
		prefilteredMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		prefilteredMapImageInfo.imageView = m_PrefilteredMap->GetVulkanImageView();
		prefilteredMapImageInfo.sampler = m_MultiLODSampler;

		{
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.anisotropyEnable = VK_TRUE;

			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(VulkanContext::GetPhysicalDevice(), &properties);

			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 1.0f;

			VK_CHECK_RESULT(vkCreateSampler(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), &samplerInfo, nullptr, &m_BRDFLUTSampler));
		}

		VkDescriptorImageInfo brdflutMapImageInfo{};
		brdflutMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		brdflutMapImageInfo.imageView = m_BRDFLUTMap->GetVulkanImageView();
		brdflutMapImageInfo.sampler = m_BRDFLUTSampler;

		m_SkymapDescriptorSet->WriteImage(0, skymapImageInfo);
		m_SkymapDescriptorSet->WriteImage(1, irrImageInfo);
		m_SkymapDescriptorSet->WriteImage(2, prefilteredMapImageInfo);
		m_SkymapDescriptorSet->WriteImage(3, brdflutMapImageInfo);
		m_SkymapDescriptorSet->Update();

		// TODO: Remove this in the future
		VulkanContext::GetCurrentDevice()->WaitIdle();

		const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		// Destroy temporary resources
		for (const auto& imageView : prefilteredMipMapImageViews)
			vkDestroyImageView(device, imageView, nullptr);
	}

	Skymap::~Skymap()
	{
		const auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vkDestroySampler(device, m_BRDFLUTSampler, nullptr);
		vkDestroySampler(device, m_MultiLODSampler, nullptr);
	}

	void Skymap::Init()
	{
		// Creation of the destination cubemap where our main skymap will be stored
		ImageSpecification imageSpec;
		imageSpec.Width = 1;
		imageSpec.Height = 1;
		imageSpec.Format = VK_FORMAT_R32G32B32A32_SFLOAT;
		// 6 Layers for 6 faces
		imageSpec.ArrayLayers = 6;
		imageSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		// Storage Image for the compute shaders to store data from equirectangular map
		// Sampled by the PBR fragment shader for rendering
		imageSpec.Usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		// Important flag to ensure that we can sample the image as a cube
		imageSpec.Flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		imageSpec.ViewSpecification.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		// VkImageView type should be VK_IMAGE_VIEW_TYPE_CUBE
		imageSpec.ViewSpecification.ViewType = VK_IMAGE_VIEW_TYPE_CUBE;
		// 6 Layers for 6 faces
		imageSpec.ViewSpecification.LayerCount = 6;

		// Creation of the cubemap image
		s_EmptyCubemap = CreateRef<Image>(imageSpec);

		// Avoid Validation Errors while accessing an empty image
		s_EmptyCubemap->TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		DescriptorSetLayoutSpecification descSetLayoutSpec;
		descSetLayoutSpec.Bindings = {
			{ .binding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
			{ .binding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
			{ .binding = 2, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT },
			{ .binding = 3, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT }
		};

		Ref<DescriptorSetLayout> descSetLayout = DescriptorSetLayout::CreateOrGetCached(descSetLayoutSpec);

		DescriptorSetSpecification descSetSpec;
		descSetSpec.Layout = descSetLayout;

		s_EmptyDescriptorSet = CreateRef<DescriptorSet>(descSetSpec);

		VkDescriptorImageInfo skymapImageInfo{};
		skymapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		skymapImageInfo.imageView = s_EmptyCubemap->GetVulkanImageView();
		skymapImageInfo.sampler = Texture2D::GetDefaultSampler();

		VkDescriptorImageInfo empty2DTextureInfo{};
		empty2DTextureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		empty2DTextureInfo.imageView = Texture2D::GetEmptyImageView();
		empty2DTextureInfo.sampler = Texture2D::GetDefaultSampler();

		s_EmptyDescriptorSet->WriteImage(0, skymapImageInfo);
		s_EmptyDescriptorSet->WriteImage(1, skymapImageInfo);
		s_EmptyDescriptorSet->WriteImage(2, skymapImageInfo);
		s_EmptyDescriptorSet->WriteImage(3, empty2DTextureInfo);
		s_EmptyDescriptorSet->Update();
	}

	void Skymap::Destroy()
	{
		s_CubemapSizeToBRDFLUTMap.clear();
		s_EmptyCubemap = nullptr;
		s_EmptyDescriptorSet = nullptr;
	}

} // namespace Flameberry
