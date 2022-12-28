#pragma once

#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.h>

namespace Flameberry {
    class VulkanDescriptorPool
    {
    public:
        VulkanDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~VulkanDescriptorPool();

        VkDescriptorPool GetDescriptorPool() { return m_VkDescriptorPool; }
        bool AllocateDescriptorSet(VkDescriptorSet* descriptorSet, VkDescriptorSetLayout descriptorSetLayout);
    private:
        VkDescriptorPool m_VkDescriptorPool = VK_NULL_HANDLE;
    };

    class VulkanDescriptorLayout
    {
    public:
        VulkanDescriptorLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
        ~VulkanDescriptorLayout();

        VkDescriptorSetLayout GetLayout() const { return m_VkDescriptorSetLayout; }
    private:
        VkDescriptorSetLayout m_VkDescriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorType> m_DescriptorTypeMap;

        friend class VulkanDescriptorWriter;
    };

    class VulkanDescriptorWriter
    {
    public:
        VulkanDescriptorWriter(VulkanDescriptorLayout& descriptorLayout);
        ~VulkanDescriptorWriter();

        void WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        void WriteImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);
        void Update(VkDescriptorSet& descriptorSet);
    private:
        std::vector<VkWriteDescriptorSet> m_VkWrites;
        VulkanDescriptorLayout& m_DescriptorLayout;
    };
}
