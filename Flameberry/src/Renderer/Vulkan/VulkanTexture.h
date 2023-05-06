#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "VulkanImage.h"
#include "VulkanDescriptor.h"

namespace Flameberry {
    class VulkanTexture
    {
    public:
        VulkanTexture(const char* texturePath, VkSampler sampler = VK_NULL_HANDLE);
        ~VulkanTexture();

        std::string GetFilePath() const { return m_FilePath; }

        VkImageView GetImageView() const { return m_TextureImage->GetImageView(); }
        VkSampler GetSampler() const { return m_VkTextureSampler; }
        VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

        // This function is to be used when initializing the Vulkan Renderer
        static void InitStaticResources();
        static void DestroyStaticResources();

        static std::shared_ptr<VulkanDescriptorLayout> GetDescriptorLayout() { return s_DescriptorLayout; }
        static VkDescriptorSet GetEmptyDescriptorSet() { return s_EmptyDescriptorSet; }
        static VkSampler GetDefaultSampler() { return s_DefaultSampler; }

        static std::shared_ptr<VulkanTexture> TryGetOrLoadTexture(const std::string& texturePath);
    private:
        std::string m_FilePath;

        std::unique_ptr<VulkanImage> m_TextureImage;
        VkSampler m_VkTextureSampler;
        VkDescriptorSet m_DescriptorSet;

        bool m_DidCreateSampler = true;

        static std::shared_ptr<VulkanDescriptorLayout> s_DescriptorLayout;
        static VkDescriptorSet s_EmptyDescriptorSet;
        static std::shared_ptr<VulkanImage> s_EmptyImage;
        static VkSampler s_DefaultSampler;

        static std::unordered_map<std::string, std::shared_ptr<VulkanTexture>> s_TextureCacheDirectory;
    };
}
