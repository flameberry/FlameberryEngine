#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Image.h"

namespace Flameberry {
    struct FramebufferSpecification
    {
        uint32_t Width, Height;
        std::vector<VkFormat> Attachments;
        uint32_t Samples;
        VkClearColorValue ClearColorValue;
        VkClearDepthStencilValue DepthStencilClearValue;
        VkAttachmentLoadOp ColorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, DepthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentStoreOp ColorStoreOp = VK_ATTACHMENT_STORE_OP_STORE, DepthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    };

    class Framebuffer
    {
    public:
        Framebuffer(const FramebufferSpecification& specification);
        ~Framebuffer();

        void Resize(uint32_t width, uint32_t height, VkRenderPass renderPass);

        FramebufferSpecification GetSpecification() const { return m_FramebufferSpec; }
        VkFramebuffer GetVulkanFramebuffer() const { return m_VkFramebuffer; }
        VkImageView GetAttachmentImageView(uint32_t attachmentIndex) const { return m_FramebufferImages[attachmentIndex]->GetImageView(); }
        VkImage GetAttachmentImage(uint32_t attachmentIndex) const { return m_FramebufferImages[attachmentIndex]->GetImage(); }

        void SetClearColorValue(const VkClearColorValue& value) { m_FramebufferSpec.ClearColorValue = value; }

        void CreateVulkanFramebuffer(VkRenderPass renderPass);

        template<typename... Args>
        static std::shared_ptr<Framebuffer> Create(Args... args) { return std::make_shared<Framebuffer>(std::forward<Args>(args)...); }
    private:
        void Invalidate();
    private:
        std::vector<std::shared_ptr<Image>> m_FramebufferImages;

        FramebufferSpecification m_FramebufferSpec;
        VkFramebuffer m_VkFramebuffer = VK_NULL_HANDLE;
    };
}
