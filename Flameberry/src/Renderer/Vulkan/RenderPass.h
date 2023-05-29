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

        void Begin(VkCommandBuffer commandBuffer, uint32_t framebufferInstance = 0, VkOffset2D renderAreaOffset = { 0, 0 }, VkExtent2D renderAreaExtent = { 0, 0 });
        void End(VkCommandBuffer commandBuffer);

        RenderPassSpecification GetSpecification() const { return m_RenderPassSpec; }
        VkRenderPass GetRenderPass() const { return m_VkRenderPass; }

        template<typename... Args>
        static std::shared_ptr<RenderPass> Create(Args... args) { return std::make_shared<RenderPass>(std::forward<Args>(args)...); }
    private:
        RenderPassSpecification m_RenderPassSpec;
        VkRenderPass m_VkRenderPass;
    };
}