#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Renderer/Vulkan/VulkanRenderer.h"

namespace Flameberry {
    class ImGuiLayer
    {
    public:
        void OnAttach(const std::shared_ptr<VulkanRenderer>& renderer);
        void OnDetach();
        void Begin();
        void End(VkCommandBuffer commandBuffer, uint32_t currentFrameIndex, VkExtent2D extent);
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