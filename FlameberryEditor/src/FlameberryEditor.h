#pragma once

#include "Flameberry.h"

namespace Flameberry {
    class FlameberryEditor : public Application
    {
    public:
        FlameberryEditor();
        virtual ~FlameberryEditor();
        void OnUpdate(float delta) override;
        void OnUIRender() override;
        void InvalidateViewportImGuiDescriptorSet();
    private:
        std::shared_ptr<VulkanRenderer> m_VulkanRenderer;

        PerspectiveCamera m_ActiveCamera;

        std::unique_ptr<VulkanDescriptorWriter> m_VulkanDescriptorWriter;
        std::unique_ptr<VulkanDescriptorLayout> m_VulkanDescriptorLayout;

        std::vector<VkDescriptorSet> m_VkDescriptorSets;

        std::unique_ptr<VulkanBuffer> m_UniformBuffers[VulkanSwapChain::MAX_FRAMES_IN_FLIGHT];

        std::unique_ptr<VulkanTexture> m_Texture;
        std::unique_ptr<MeshRenderer> m_MeshRenderer;
        std::vector<std::shared_ptr<VulkanMesh>> m_Meshes;

        // Test
        std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
        glm::vec2 m_ViewportSize{1280, 720};
        VkSampler m_VkTextureSampler;
        std::vector<VkDescriptorSet> m_ViewportDescriptorSets;
    };
}