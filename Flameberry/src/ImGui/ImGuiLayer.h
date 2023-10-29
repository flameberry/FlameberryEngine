#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Core/Layer.h"

namespace Flameberry {
    class ImGuiLayer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer() = default;

        void OnDestroy();
        void Begin();
        void End();

        void OnEvent(Event& e);
        void InvalidateResources();
    private:
        void SetupImGuiStyle();
        void CreateResources();
    private:
        VkRenderPass m_ImGuiLayerRenderPass;
        std::vector<VkFramebuffer> m_ImGuiFramebuffers;
        std::vector<VkImageView> m_ImGuiImageViews;
    };
}