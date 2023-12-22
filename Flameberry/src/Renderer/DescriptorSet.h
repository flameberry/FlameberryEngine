#pragma once

#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "Core/Core.h"

namespace Flameberry {
    class DescriptorPool
    {
    public:
        DescriptorPool(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
        ~DescriptorPool();

        VkDescriptorPool GetVulkanDescriptorPool() { return m_VkDescriptorPool; }
        bool AllocateDescriptorSet(VkDescriptorSet* descriptorSet, VkDescriptorSetLayout descriptorSetLayout);
    private:
        VkDescriptorPool m_VkDescriptorPool = VK_NULL_HANDLE;
    };

    struct DescriptorSetLayoutSpecification
    {
        std::vector<VkDescriptorSetLayoutBinding> Bindings;
    };

    class DescriptorSetLayout
    {
    public:
        DescriptorSetLayout(const DescriptorSetLayoutSpecification& specification);
        ~DescriptorSetLayout();

        static Ref<DescriptorSetLayout> CreateOrGetCached(const DescriptorSetLayoutSpecification& specification);
        static void ClearCache();

        VkDescriptorSetLayout GetLayout() const { return m_Layout; }
        DescriptorSetLayoutSpecification GetSpecification() const { return m_DescSetLayoutSpec; }
    private:
        DescriptorSetLayoutSpecification m_DescSetLayoutSpec;
        VkDescriptorSetLayout m_Layout;

        static std::unordered_map<DescriptorSetLayoutSpecification, Ref<DescriptorSetLayout>> s_CachedDescriptorSetLayouts;
    };

    struct DescriptorSetSpecification
    {
        Ref<DescriptorPool> Pool;
        Ref<DescriptorSetLayout> Layout;
    };

    class DescriptorSet
    {
    public:
        DescriptorSet(const DescriptorSetSpecification& specification);
        ~DescriptorSet();

        DescriptorSetSpecification GetSpecification() const { return m_DescSetSpec; }
        VkDescriptorSet GetVulkanDescriptorSet() const { return m_DescriptorSet; }

        void WriteBuffer(uint32_t binding, VkDescriptorBufferInfo& bufferInfo);
        void WriteImage(uint32_t binding, VkDescriptorImageInfo& imageInfo);
        void Update();
    private:
        std::vector<VkWriteDescriptorSet> m_WriteInfos;

        DescriptorSetSpecification m_DescSetSpec;
        VkDescriptorSet m_DescriptorSet;
    };
}
