#pragma once

#include "Flameberry.h"

class SandboxApp: public Flameberry::Application
{
public:
    SandboxApp();
    virtual ~SandboxApp();
    void OnUpdate(float delta) override;
    void OnUIRender() override;
private:
    Flameberry::PerspectiveCamera m_ActiveCamera;
    std::unique_ptr<Flameberry::VulkanPipeline> m_VulkanPipeline;
    VkPipelineLayout m_VkPipelineLayout;

    std::unique_ptr<Flameberry::VulkanDescriptorPool> m_VulkanDescriptorPool;
    std::unique_ptr<Flameberry::VulkanDescriptorWriter> m_VulkanDescriptorWriter;
    std::unique_ptr<Flameberry::VulkanDescriptorLayout> m_VulkanDescriptorLayout;

    std::vector<VkDescriptorSet> m_VkDescriptorSets;

    std::unique_ptr<Flameberry::VulkanBuffer> m_VertexBuffer, m_IndexBuffer;
    std::unique_ptr<Flameberry::VulkanBuffer> m_UniformBuffers[MAX_FRAMES_IN_FLIGHT];
    uint32_t m_Indices[MAX_INDICES];

    std::unique_ptr<Flameberry::VulkanTexture> m_Texture;
};
