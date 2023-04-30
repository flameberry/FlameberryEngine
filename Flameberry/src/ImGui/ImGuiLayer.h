#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Core/Layer.h"
#include "Renderer/Vulkan/VulkanRenderer.h"

namespace Flameberry {
    class ImGuiLayer
    {
    public:
        ImGuiLayer(const std::shared_ptr<VulkanRenderer>& renderer);
        ~ImGuiLayer() = default;

        void OnDestroy();
        void Begin();
        void End(VkCommandBuffer commandBuffer, uint32_t currentFrameIndex, VkExtent2D extent);

        void OnEvent(Event& e);
        void InvalidateResources(const std::shared_ptr<VulkanRenderer>& renderer);
    private:
        void SetupImGuiStyle();
        void CreateResources(const std::shared_ptr<VulkanRenderer>& renderer);
    private:
        VkRenderPass m_ImGuiLayerRenderPass;
        std::vector<VkFramebuffer> m_ImGuiFramebuffers;
        std::vector<VkImageView> m_ImGuiImageViews;
    };
}