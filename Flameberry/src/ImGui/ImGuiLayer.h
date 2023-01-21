#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Flameberry {
    class ImGuiLayer
    {
    public:
        void OnAttach(VkDescriptorPool descriptorPool, VkFormat swapChainImageFormat, const std::vector<VkImageView>& imageViews, VkExtent2D extent);
        void OnDetach();
        void Begin();
        void End(VkCommandBuffer commandBuffer, uint32_t currentFrameIndex, VkExtent2D extent);
    private:
        void SetupImGuiStyle();
    private:
        VkRenderPass m_ImGuiLayerRenderPass;
        std::vector<VkFramebuffer> m_ImGuiFramebuffers;
    };
}