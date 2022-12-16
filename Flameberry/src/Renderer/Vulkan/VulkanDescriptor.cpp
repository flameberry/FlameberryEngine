#include "VulkanDescriptor.h"

#include <array>

#include "Core/Core.h"
#include "VulkanRenderer.h"

namespace Flameberry {
    VulkanDescriptorPool::VulkanDescriptorPool(VkDevice& device)
        : m_VkDevice(device)
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo vk_descriptor_pool_create_info{};
        vk_descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        vk_descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        vk_descriptor_pool_create_info.pPoolSizes = poolSizes.data();
        vk_descriptor_pool_create_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        FL_ASSERT(vkCreateDescriptorPool(m_VkDevice, &vk_descriptor_pool_create_info, nullptr, &m_VkDescriptorPool) == VK_SUCCESS, "Failed to create Vulkan Descriptor Pool!");
    }

    VulkanDescriptorPool::~VulkanDescriptorPool()
    {
        vkDestroyDescriptorPool(m_VkDevice, m_VkDescriptorPool, nullptr);
    }

    bool VulkanDescriptorPool::AllocateDescriptorSet(VkDescriptorSet* descriptorSet, VkDescriptorSetLayout descriptorSetLayout)
    {
        VkDescriptorSetAllocateInfo vk_descriptor_set_allocate_info{};
        vk_descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        vk_descriptor_set_allocate_info.descriptorPool = m_VkDescriptorPool;
        vk_descriptor_set_allocate_info.descriptorSetCount = 1;
        vk_descriptor_set_allocate_info.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(m_VkDevice, &vk_descriptor_set_allocate_info, descriptorSet) != VK_SUCCESS)
        {
            FL_ERROR("Failed to allocate Vulkan Descriptor Sets!");
            return false;
        }
        return true;
    }

    VulkanDescriptorWriter::VulkanDescriptorWriter(VkDevice& device, VulkanDescriptorLayout& descriptorLayout)
        : m_VkDevice(device), m_DescriptorLayout(descriptorLayout)
    {
    }

    void VulkanDescriptorWriter::WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo)
    {
        VkWriteDescriptorSet vk_write_descriptor_set{};
        vk_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vk_write_descriptor_set.dstBinding = binding;
        vk_write_descriptor_set.dstArrayElement = 0;
        vk_write_descriptor_set.descriptorType = m_DescriptorLayout.m_DescriptorTypeMap[binding];
        vk_write_descriptor_set.descriptorCount = 1;
        vk_write_descriptor_set.pBufferInfo = bufferInfo;
        vk_write_descriptor_set.pImageInfo = nullptr;
        vk_write_descriptor_set.pTexelBufferView = nullptr;

        m_VkWrites.push_back(vk_write_descriptor_set);
    }

    void VulkanDescriptorWriter::WriteImage(uint32_t binding, VkDescriptorImageInfo* imageInfo)
    {
        VkWriteDescriptorSet vk_write_descriptor_set{};
        vk_write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vk_write_descriptor_set.dstBinding = binding;
        vk_write_descriptor_set.dstArrayElement = 0;
        vk_write_descriptor_set.descriptorType = m_DescriptorLayout.m_DescriptorTypeMap[binding];
        vk_write_descriptor_set.descriptorCount = 1;
        vk_write_descriptor_set.pBufferInfo = nullptr;
        vk_write_descriptor_set.pImageInfo = imageInfo;
        vk_write_descriptor_set.pTexelBufferView = nullptr;

        m_VkWrites.push_back(vk_write_descriptor_set);
    }

    void VulkanDescriptorWriter::Update(VkDescriptorSet& descriptorSet)
    {
        for (auto& write : m_VkWrites)
            write.dstSet = descriptorSet;
        vkUpdateDescriptorSets(m_VkDevice, static_cast<uint32_t>(m_VkWrites.size()), m_VkWrites.data(), 0, nullptr);
    }

    VulkanDescriptorWriter::~VulkanDescriptorWriter()
    {
    }

    VulkanDescriptorLayout::VulkanDescriptorLayout(VkDevice& device, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
        : m_VkDevice(device)
    {
        for (const auto& binding : bindings)
            m_DescriptorTypeMap[binding.binding] = binding.descriptorType;

        VkDescriptorSetLayoutCreateInfo vk_descriptor_set_layout_create_info{};
        vk_descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        vk_descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(bindings.size());
        vk_descriptor_set_layout_create_info.pBindings = bindings.data();

        FL_ASSERT(vkCreateDescriptorSetLayout(m_VkDevice, &vk_descriptor_set_layout_create_info, nullptr, &m_VkDescriptorSetLayout) == VK_SUCCESS, "Failed to create Vulkan Descriptor Set Layout!");
    }

    VulkanDescriptorLayout::~VulkanDescriptorLayout()
    {
        vkDestroyDescriptorSetLayout(m_VkDevice, m_VkDescriptorSetLayout, nullptr);
    }
}
