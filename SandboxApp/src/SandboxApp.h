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
    std::shared_ptr<Flameberry::VulkanRenderer> m_VulkanRenderer;

    Flameberry::PerspectiveCamera m_ActiveCamera;

    std::unique_ptr<Flameberry::VulkanDescriptorWriter> m_VulkanDescriptorWriter;
    std::unique_ptr<Flameberry::VulkanDescriptorLayout> m_VulkanDescriptorLayout;

    std::vector<VkDescriptorSet> m_VkDescriptorSets;

    std::unique_ptr<Flameberry::VulkanBuffer> m_UniformBuffers[Flameberry::VulkanSwapChain::MAX_FRAMES_IN_FLIGHT];

    std::unique_ptr<Flameberry::VulkanTexture> m_Texture;
    std::unique_ptr<Flameberry::MeshRenderer> m_MeshRenderer;
    std::vector<std::shared_ptr<Flameberry::VulkanMesh>> m_Meshes;
};
