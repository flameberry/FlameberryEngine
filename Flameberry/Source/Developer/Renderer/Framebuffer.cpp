#include "Framebuffer.h"

#include "Renderer/Image.h"
#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "RenderCommand.h"
#include "vulkan/vulkan_core.h"

namespace Flameberry {

	Framebuffer::Framebuffer(const FramebufferSpecification& specification) : m_FramebufferSpec(specification) {
		Invalidate();
	}

	void Framebuffer::OnResize(uint32_t width, uint32_t height, VkRenderPass renderPass) {
		m_FramebufferSpec.Width = width;
		m_FramebufferSpec.Height = height;
		Invalidate();
		CreateVulkanFramebuffer(renderPass);
	}

	void Framebuffer::CreateVulkanFramebuffer(VkRenderPass renderPass) {
		std::vector<VkImageView> imageViews(m_FramebufferImages.size());
		for (uint32_t i = 0; i < m_FramebufferImages.size(); i++)
			imageViews[i] = m_FramebufferImages[i]->GetVulkanImageView();

		VkFramebufferCreateInfo vk_framebuffer_create_info{};
		vk_framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		vk_framebuffer_create_info.renderPass = renderPass;
		vk_framebuffer_create_info.attachmentCount = static_cast<uint32_t>(m_FramebufferImages.size());
		vk_framebuffer_create_info.pAttachments = imageViews.data();
		vk_framebuffer_create_info.width = m_FramebufferSpec.Width;
		vk_framebuffer_create_info.height = m_FramebufferSpec.Height;
		vk_framebuffer_create_info.layers = 1;

		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VK_CHECK_RESULT(vkCreateFramebuffer(device, &vk_framebuffer_create_info, nullptr, &m_VulkanFramebuffer));
	}

	void Framebuffer::Invalidate() {
		if (m_VulkanFramebuffer != VK_NULL_HANDLE) {
			const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			VulkanContext::GetCurrentDevice()->WaitIdle();
			vkDestroyFramebuffer(device, m_VulkanFramebuffer, nullptr);
			m_FramebufferImages.clear();
		}

		std::vector<FramebufferAttachmentSpecification> colorAttachments;

		for (const auto& attachment : m_FramebufferSpec.Attachments) {
			ImageSpecification imageSpec;
			imageSpec.Width = m_FramebufferSpec.Width;
			imageSpec.Height = m_FramebufferSpec.Height;
			imageSpec.MipLevels = 1;
			imageSpec.ArrayLayers = attachment.LayerCount;
			imageSpec.Samples = m_FramebufferSpec.Samples;
			imageSpec.Format = attachment.Format;
			imageSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			// TODO: Remove setting usage as VK_IMAGE_USAGE_TRANSFER_SRC_BIT for all
			imageSpec.Usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			imageSpec.Views = { {} };

			ImageViewSpecification& mainView = imageSpec.Views[0];
			mainView.Type = attachment.LayerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;

			if (RenderCommand::DoesFormatSupportDepthAttachment(attachment.Format)) {
				mainView.AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
				imageSpec.Usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				m_DepthAttachmentIndex = m_FramebufferImages.size();

				if (RenderCommand::DoesFormatSupportStencilAttachment(attachment.Format))
					imageSpec.Views.push_back({ .AspectFlags = VK_IMAGE_ASPECT_STENCIL_BIT });
			} else {
				mainView.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
				imageSpec.Usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

				colorAttachments.emplace_back(attachment);
			}

			m_FramebufferImages.emplace_back(CreateRef<Image>(imageSpec));
		}

		if (m_FramebufferSpec.Samples > 1) {
			for (auto& attachment : colorAttachments) {
				ImageSpecification imageSpec;
				imageSpec.Width = m_FramebufferSpec.Width;
				imageSpec.Height = m_FramebufferSpec.Height;
				imageSpec.MipLevels = 1;
				imageSpec.ArrayLayers = attachment.LayerCount;
				imageSpec.Samples = 1;
				imageSpec.Format = attachment.Format;
				imageSpec.Usage =
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
				imageSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				imageSpec.Views = { { .AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT } };

				m_FramebufferImages.emplace_back(CreateRef<Image>(imageSpec));
			}
		}
	}

	Framebuffer::~Framebuffer() {
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vkDestroyFramebuffer(device, m_VulkanFramebuffer, nullptr);
	}

} // namespace Flameberry
