#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Core/Layer.h"

namespace Flameberry {
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		void OnCreate() override;
		void OnUpdate(float delta) override {};
		void OnUIRender() override {};
		void OnEvent(Event& e) override;
		void OnDestroy() override;

		void Begin();
		void End();

		void BlockEvents(bool block) { m_BlockEvents = block; }
		void InvalidateResources();

	private:
		void SetupImGuiStyle();
		void CreateResources();

	private:
		bool					   m_BlockEvents = false;
		VkRenderPass			   m_ImGuiLayerRenderPass;
		std::vector<VkFramebuffer> m_ImGuiFramebuffers;
		std::vector<VkImageView>   m_ImGuiImageViews;
	};
} // namespace Flameberry