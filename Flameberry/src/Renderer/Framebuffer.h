#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Image.h"

namespace Flameberry {
	struct FramebufferAttachmentSpecification
	{
		VkFormat Format;
		uint32_t LayerCount;

		FramebufferAttachmentSpecification(VkFormat format)
			: Format(format), LayerCount(1) {}
		FramebufferAttachmentSpecification(VkFormat format, uint32_t layerCount)
			: Format(format), LayerCount(layerCount) {}

		bool operator==(const FramebufferAttachmentSpecification& other) const
		{
			return this->Format == other.Format && this->LayerCount == other.LayerCount;
		}

		bool operator!=(const FramebufferAttachmentSpecification& other) const
		{
			return !(*this == other);
		}
	};

	struct FramebufferSpecification
	{
		uint32_t										Width, Height;
		std::vector<FramebufferAttachmentSpecification> Attachments;
		uint32_t										Samples;
		VkClearColorValue								ClearColorValue;
		VkClearDepthStencilValue						DepthStencilClearValue;
		VkAttachmentLoadOp								ColorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, DepthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		VkAttachmentStoreOp								ColorStoreOp = VK_ATTACHMENT_STORE_OP_STORE, DepthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	};

	class Framebuffer
	{
	public:
		Framebuffer(const FramebufferSpecification& specification);
		~Framebuffer();

		void CreateVulkanFramebuffer(VkRenderPass renderPass);
		void OnResize(uint32_t width, uint32_t height, VkRenderPass renderPass);

		FramebufferSpecification GetSpecification() const { return m_FramebufferSpec; }
		VkFramebuffer			 GetVulkanFramebuffer() const { return m_VkFramebuffer; }

		Ref<Image> GetColorAttachment(uint32_t attachmentIndex) const { return m_FramebufferImages[attachmentIndex]; }
		Ref<Image> GetColorResolveAttachment(uint32_t attachmentIndex) const { return m_FramebufferImages[m_DepthAttachmentIndex + 1 + attachmentIndex]; }
		Ref<Image> GetDepthAttachment() const { return m_FramebufferImages[m_DepthAttachmentIndex]; }

		void SetClearColorValue(const VkClearColorValue& value) { m_FramebufferSpec.ClearColorValue = value; }

	private:
		void Invalidate();

	private:
		std::vector<Ref<Image>> m_FramebufferImages;
		uint32_t				m_DepthAttachmentIndex = -1;

		FramebufferSpecification m_FramebufferSpec;
		VkFramebuffer			 m_VkFramebuffer = VK_NULL_HANDLE;
	};
} // namespace Flameberry
