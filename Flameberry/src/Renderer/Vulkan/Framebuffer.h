#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Image.h"

namespace Flameberry {
    struct FramebufferAttachmentSpecification
    {
        VkFormat Format;
        uint32_t LayerCount;

        FramebufferAttachmentSpecification(VkFormat format) : Format(format), LayerCount(1) {}
        FramebufferAttachmentSpecification(VkFormat format, uint32_t layerCount) : Format(format), LayerCount(layerCount) {}

        bool operator==(const FramebufferAttachmentSpecification& other) const {
            return this->Format == other.Format && this->LayerCount == other.LayerCount;
        }

        bool operator!=(const FramebufferAttachmentSpecification& other) const {
            return !(*this == other);
        }
    };

    struct FramebufferSpecification
    {
        uint32_t Width, Height;
        std::vector<FramebufferAttachmentSpecification> Attachments;
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

        void CreateVulkanFramebuffer(VkRenderPass renderPass);
        bool Resize(uint32_t width, uint32_t height, VkRenderPass renderPass);

        FramebufferSpecification GetSpecification() const { return m_FramebufferSpec; }
        VkFramebuffer GetVulkanFramebuffer() const { return m_VkFramebuffer; }

        std::shared_ptr<Image> GetColorAttachment(uint32_t attachmentIndex) const { return m_FramebufferImages[attachmentIndex]; }
        std::shared_ptr<Image> GetColorResolveAttachment(uint32_t attachmentIndex) const { return m_FramebufferImages[m_DepthAttachmentIndex + 1 + attachmentIndex]; }
        std::shared_ptr<Image> GetDepthAttachment() const { return m_FramebufferImages[m_DepthAttachmentIndex]; }

        void SetClearColorValue(const VkClearColorValue& value) { m_FramebufferSpec.ClearColorValue = value; }

        template<typename... Args>
        static std::shared_ptr<Framebuffer> Create(Args... args) { return std::make_shared<Framebuffer>(std::forward<Args>(args)...); }
    private:
        void Invalidate();
    private:
        std::vector<std::shared_ptr<Image>> m_FramebufferImages;
        uint32_t m_DepthAttachmentIndex = -1;

        FramebufferSpecification m_FramebufferSpec;
        VkFramebuffer m_VkFramebuffer = VK_NULL_HANDLE;
    };
}
