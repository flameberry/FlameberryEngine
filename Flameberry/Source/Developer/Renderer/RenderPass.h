#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Framebuffer.h"

namespace Flameberry {

	struct RenderPassSpecification
	{
		std::vector<Ref<Framebuffer>> TargetFramebuffers;
		std::vector<VkSubpassDependency> Dependencies;
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

	private:
		RenderPassSpecification m_RenderPassSpec;
		VkRenderPass m_VkRenderPass;
	};

} // namespace Flameberry
