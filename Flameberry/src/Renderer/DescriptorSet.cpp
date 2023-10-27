#include "DescriptorSet.h"

#include <array>

#include "SwapChain.h"
#include "VulkanContext.h"
#include "RenderCommand.h"
#include "VulkanDebug.h"

namespace Flameberry {
    DescriptorPool::DescriptorPool(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets)
    {
        VkDescriptorPoolCreateInfo vk_descriptor_pool_create_info{};
        vk_descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        vk_descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        vk_descriptor_pool_create_info.pPoolSizes = poolSizes.data();
        vk_descriptor_pool_create_info.maxSets = maxSets;
        vk_descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &vk_descriptor_pool_create_info, nullptr, &m_VkDescriptorPool));
    }

    DescriptorPool::~DescriptorPool()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyDescriptorPool(device, m_VkDescriptorPool, nullptr);
    }

    bool DescriptorPool::AllocateDescriptorSet(VkDescriptorSet* descriptorSet, VkDescriptorSetLayout descriptorSetLayout)
    {
        VkDescriptorSetAllocateInfo vk_descriptor_set_allocate_info{};
        vk_descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        vk_descriptor_set_allocate_info.descriptorPool = m_VkDescriptorPool;
        vk_descriptor_set_allocate_info.descriptorSetCount = 1;
        vk_descriptor_set_allocate_info.pSetLayouts = &descriptorSetLayout;

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &vk_descriptor_set_allocate_info, descriptorSet));
        return true;
    }

    DescriptorSetLayout::DescriptorSetLayout(const DescriptorSetLayoutSpecification& specification)
        : m_DescSetLayoutSpec(specification)
    {
        VkDescriptorSetLayoutCreateInfo vk_descriptor_set_layout_create_info{};
        vk_descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        vk_descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(m_DescSetLayoutSpec.Bindings.size());
        vk_descriptor_set_layout_create_info.pBindings = m_DescSetLayoutSpec.Bindings.data();

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &vk_descriptor_set_layout_create_info, nullptr, &m_Layout));
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyDescriptorSetLayout(device, m_Layout, nullptr);
    }

    DescriptorSet::DescriptorSet(const DescriptorSetSpecification& specification)
        : m_DescSetSpec(specification)
    {
        auto layout = m_DescSetSpec.Layout->GetLayout();

        VkDescriptorSetAllocateInfo vk_descriptor_set_allocate_info{};
        vk_descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        vk_descriptor_set_allocate_info.descriptorPool = m_DescSetSpec.Pool ? m_DescSetSpec.Pool->GetVulkanDescriptorPool() : VulkanContext::GetCurrentGlobalDescriptorPool()->GetVulkanDescriptorPool();
        vk_descriptor_set_allocate_info.descriptorSetCount = 1;
        vk_descriptor_set_allocate_info.pSetLayouts = &layout;

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &vk_descriptor_set_allocate_info, &m_DescriptorSet));
    }

    DescriptorSet::~DescriptorSet()
    {
    }

    void DescriptorSet::WriteBuffer(uint32_t binding, VkDescriptorBufferInfo& bufferInfo)
    {
        VkWriteDescriptorSet vk_write_descriptor_set{};
        vk_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vk_write_descriptor_set.dstSet = m_DescriptorSet;
        vk_write_descriptor_set.dstBinding = binding;
        vk_write_descriptor_set.dstArrayElement = 0;
        vk_write_descriptor_set.descriptorType = m_DescSetSpec.Layout->GetSpecification().Bindings[binding].descriptorType;
        vk_write_descriptor_set.descriptorCount = 1;
        vk_write_descriptor_set.pBufferInfo = &bufferInfo;
        vk_write_descriptor_set.pImageInfo = nullptr;
        vk_write_descriptor_set.pTexelBufferView = nullptr;

        m_WriteInfos.push_back(vk_write_descriptor_set);
    }

    void DescriptorSet::WriteImage(uint32_t binding, VkDescriptorImageInfo& imageInfo)
    {
        VkWriteDescriptorSet vk_write_descriptor_set{};
        vk_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vk_write_descriptor_set.dstSet = m_DescriptorSet;
        vk_write_descriptor_set.dstBinding = binding;
        vk_write_descriptor_set.dstArrayElement = 0;
        vk_write_descriptor_set.descriptorType = m_DescSetSpec.Layout->GetSpecification().Bindings[binding].descriptorType;
        vk_write_descriptor_set.descriptorCount = 1;
        vk_write_descriptor_set.pBufferInfo = nullptr;
        vk_write_descriptor_set.pImageInfo = &imageInfo;
        vk_write_descriptor_set.pTexelBufferView = nullptr;

        m_WriteInfos.push_back(vk_write_descriptor_set);
    }

    void DescriptorSet::Update()
    {
        if (m_WriteInfos.size())
        {
            FL_LOG("Size of Write Infos: {0}", m_WriteInfos.size());
            for (auto& info : m_WriteInfos)
            {
                if (info.pBufferInfo)
                    FL_LOG("Buffer: {0}", info.pBufferInfo->buffer);
                else if (info.pImageInfo)
                    FL_LOG("Image: {0}", info.pImageInfo->imageView);
            }
            const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_WriteInfos.size()), m_WriteInfos.data(), 0, nullptr);
            m_WriteInfos.clear();
        }
    }
}
