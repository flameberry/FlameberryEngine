#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Framebuffer.h"

namespace Flameberry {
    struct RenderPassSpecification
    {
        std::vector<std::shared_ptr<Framebuffer>> TargetFramebuffers;
    };

    class RenderPass
    {
    public:
        RenderPass(const RenderPassSpecification& specification);
        ~RenderPass();

        void Begin(uint32_t framebufferInstance = -1, VkOffset2D renderAreaOffset = { 0, 0 }, VkExtent2D renderAreaExtent = { 0, 0 });
        void End();

        RenderPassSpecification GetSpecification() const { return m_RenderPassSpec; }
        VkRenderPass GetRenderPass() const { return m_VkRenderPass; }

        template<typename... Args>
        static std::shared_ptr<RenderPass> Create(Args... args) { return std::make_shared<RenderPass>(std::forward<Args>(args)...); }
    private:
        bool IsDepthAttachment(VkFormat format) const { return format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; }
    private:
        RenderPassSpecification m_RenderPassSpec;
        VkRenderPass m_VkRenderPass;
    };
}