#include "Framebuffer.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"

namespace Flameberry {
    Framebuffer::Framebuffer(const FramebufferSpecification& specification)
        : m_FramebufferSpec(specification)
    {
        Invalidate();
    }

    void Framebuffer::Resize(uint32_t width, uint32_t height, VkRenderPass renderPass)
    {
        if (width != 0 || height != 0 || m_FramebufferSpec.Width != width || m_FramebufferSpec.Height != height)
        {
            m_FramebufferSpec.Width = width;
            m_FramebufferSpec.Height = height;
            Invalidate();
            CreateVulkanFramebuffer(renderPass);
        }
    }

    void Framebuffer::CreateVulkanFramebuffer(VkRenderPass renderPass)
    {
        VkImageView imageViews[m_FramebufferImages.size()];
        for (uint32_t i = 0; i < m_FramebufferImages.size(); i++)
            imageViews[i] = m_FramebufferImages[i]->GetImageView();

        VkFramebufferCreateInfo vk_framebuffer_create_info{};
        vk_framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        vk_framebuffer_create_info.renderPass = renderPass;
        vk_framebuffer_create_info.attachmentCount = static_cast<uint32_t>(m_FramebufferImages.size());
        vk_framebuffer_create_info.pAttachments = imageViews;
        vk_framebuffer_create_info.width = m_FramebufferSpec.Width;
        vk_framebuffer_create_info.height = m_FramebufferSpec.Height;
        vk_framebuffer_create_info.layers = 1;

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &vk_framebuffer_create_info, nullptr, &m_VkFramebuffer));
    }

    void Framebuffer::Invalidate()
    {
        if (m_VkFramebuffer != VK_NULL_HANDLE)
        {
            const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
            VulkanContext::GetCurrentDevice()->WaitIdle();
            vkDestroyFramebuffer(device, m_VkFramebuffer, nullptr);

            m_FramebufferImages.clear();
        }

        std::vector<VkFormat> colorAttachmentFormats;

        for (const auto& format : m_FramebufferSpec.Attachments)
        {
            ImageSpecification imageSpec;
            imageSpec.Width = m_FramebufferSpec.Width;
            imageSpec.Height = m_FramebufferSpec.Height;
            imageSpec.MipLevels = 1;
            imageSpec.Samples = m_FramebufferSpec.Samples;
            imageSpec.Format = format;
            imageSpec.Tiling = VK_IMAGE_TILING_OPTIMAL;
            imageSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            imageSpec.Usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // TODO: Remove setting usage as VK_IMAGE_USAGE_TRANSFER_SRC_BIT for all

            if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
            {
                imageSpec.ImageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
                imageSpec.Usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            else
            {
                imageSpec.ImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
                imageSpec.Usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

                colorAttachmentFormats.emplace_back(format);
            }
            m_FramebufferImages.emplace_back(Image::Create(imageSpec));
        }

        if (m_FramebufferSpec.Samples > 1)
        {
            for (auto& format : colorAttachmentFormats)
            {
                ImageSpecification imageSpec;
                imageSpec.Width = m_FramebufferSpec.Width;
                imageSpec.Height = m_FramebufferSpec.Height;
                imageSpec.MipLevels = 1;
                imageSpec.Samples = 1;
                imageSpec.Format = format;
                imageSpec.Tiling = VK_IMAGE_TILING_OPTIMAL;
                imageSpec.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                imageSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                imageSpec.ImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

                m_FramebufferImages.emplace_back(Image::Create(imageSpec));
            }
        }
    }

    Framebuffer::~Framebuffer()
    {
        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        vkDestroyFramebuffer(device, m_VkFramebuffer, nullptr);
    }
}
