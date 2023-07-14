#pragma once

#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.h>

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

        VkDescriptorSetLayout GetLayout() const { return m_Layout; }
        DescriptorSetLayoutSpecification GetSpecification() const { return m_DescSetLayoutSpec; }

        template<typename... Args>
        static std::shared_ptr<DescriptorSetLayout> Create(Args... args) { return std::make_shared<DescriptorSetLayout>(std::forward<Args>(args)...); }
    private:
        DescriptorSetLayoutSpecification m_DescSetLayoutSpec;
        VkDescriptorSetLayout m_Layout;
    };

    struct DescriptorSetSpecification
    {
        std::shared_ptr<DescriptorPool> Pool;
        std::shared_ptr<DescriptorSetLayout> Layout;
    };

    class DescriptorSet
    {
    public:
        DescriptorSet(const DescriptorSetSpecification& specification);
        ~DescriptorSet();

        DescriptorSetSpecification GetSpecification() const { return m_DescSetSpec; }
        VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

        void WriteBuffer(uint32_t binding, const VkDescriptorBufferInfo& bufferInfo);
        void WriteImage(uint32_t binding, const VkDescriptorImageInfo& imageInfo);
        void Update();

        template<typename... Args>
        static std::shared_ptr<DescriptorSet> Create(Args... args) { return std::make_shared<DescriptorSet>(std::forward<Args>(args)...); }
    private:
        std::vector<VkWriteDescriptorSet> m_WriteInfos;

        DescriptorSetSpecification m_DescSetSpec;
        VkDescriptorSet m_DescriptorSet;
    };
}
