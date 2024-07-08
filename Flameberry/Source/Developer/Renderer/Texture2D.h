#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "Core/UUID.h"

#include "Image.h"
#include "DescriptorSet.h"
#include "Asset/Asset.h"

namespace Flameberry {

	struct Texture2DSpecification
	{
		float Width, Height;
		VkFormat Format;
		bool GenerateMipmaps = true;
		VkSampler Sampler = VK_NULL_HANDLE;
	};

	class Texture2D : public Asset
	{
	public:
		Texture2D(const std::string& texturePath, bool canGenerateMipMaps = true, VkSampler sampler = VK_NULL_HANDLE);
		Texture2D(const void* data, const Texture2DSpecification& specification);
		~Texture2D();

		VkDescriptorSet CreateOrGetDescriptorSet();
		VkImageView GetImageView() const { return m_TextureImage->GetVulkanImageView(); }
		VkSampler GetSampler() const { return m_Sampler; }
		ImageSpecification GetImageSpecification() const { return m_TextureImageSpecification; }

		FBY_DECLARE_ASSET_TYPE(AssetType::Texture2D);

		// This function is to be used when initializing the Vulkan Renderer
		static void InitStaticResources();
		static void DestroyStaticResources();

		static Ref<DescriptorSetLayout> GetDescriptorLayout() { return s_DescriptorLayout; }
		static VkDescriptorSet GetEmptyDescriptorSet() { return s_EmptyDescriptorSet->GetVulkanDescriptorSet(); }
		static VkSampler GetDefaultSampler() { return s_DefaultSampler; }
		static VkImageView GetEmptyImageView() { return s_EmptyImage->GetVulkanImageView(); }
		static Ref<Image> GetEmptyImage() { return s_EmptyImage; }

		static Ref<Texture2D> TryGetOrLoadTexture(const std::string& texturePath);

		static Ref<Texture2D> LoadFromFile(const char* path) { return CreateRef<Texture2D>(path); }

	private:
		void SetupTexture(const void* data, const float width, const float height, const float imageSize, const uint16_t mipLevels, const VkFormat format, const VkSampler sampler);

	private:
		ImageSpecification m_TextureImageSpecification;

		Ref<Image> m_TextureImage;
		VkSampler m_Sampler;
		VkDescriptorSet m_DescriptorSet;

		bool m_DidCreateSampler = false;

	private:
		static Ref<DescriptorSetLayout> s_DescriptorLayout;
		static Ref<DescriptorSet> s_EmptyDescriptorSet;
		static Ref<Image> s_EmptyImage;
		static VkSampler s_DefaultSampler;

		static std::unordered_map<std::string, Ref<Texture2D>> s_TextureCacheDirectory;
	};

} // namespace Flameberry
