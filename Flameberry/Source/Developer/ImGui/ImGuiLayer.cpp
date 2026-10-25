#include "ImGuiLayer.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <IconFontCppHeaders/IconsLucide.h>

#include "Theme.h"

#include "Renderer/RenderCommand.h"
#include "Renderer/SwapChain.h"
#include "Renderer/VulkanDebug.h"
#include "Renderer/VulkanContext.h"
#include "Renderer/Renderer.h"

namespace Flameberry {

	constexpr static const char* s_ImGuiLayoutPath = FBY_PROJECT_DIR "Flameberry/Config/Layouts/BaseEditorLayout.ini";

	ImGuiLayer::ImGuiLayer() {}

	void ImGuiLayer::OnCreate() {
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;	// Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

		// io.ConfigViewportsNoAutoMerge = true;
		// io.ConfigViewportsNoTaskBarIcon = true;
		io.ConfigDebugHighlightIdConflicts = false;

		constexpr float fontSize = 15.0f;
		static constexpr ImWchar iconRanges[] = { ICON_MIN_LC, ICON_MAX_LC, 0 };

		ImFontConfig config{};
		config.GlyphExtraAdvanceX = 0.3f;
		// Inter has its own glyphs (alternate characters) in the private use area that Lucide uses for its icons, and
		// since ImGui looks glyphs up in the order the fonts were added, they would be shown instead of the merged
		config.GlyphExcludeRanges = iconRanges;

		io.FontDefault = io.Fonts->AddFontFromFileTTF(
			FBY_PROJECT_DIR "Flameberry/Assets/Fonts/inter/Inter-VariableFont_opsz,wght.ttf", fontSize, &config);

		// Merging Lucide icons into the default font
		ImFontConfig iconFontConfig;
		iconFontConfig.MergeMode = true;
		iconFontConfig.GlyphMinAdvanceX = 13.0f; // Makes the icons monospaced
		iconFontConfig.GlyphOffset = ImVec2(0, 2.5f);

		io.Fonts->AddFontFromFileTTF(
			FBY_PROJECT_DIR "Flameberry/Assets/Fonts/lucide/lucide.ttf", fontSize, &iconFontConfig, iconRanges);

		// Scaling for high DPI monitors (the content scale is always 1 on macOS, which uses the framebuffer scale
		// instead) Fonts and viewports follow DPI changes automatically, but style sizes are baked here once
		// The style is set up first, so that its sizes get scaled too
		SetupImGuiStyle();
		const float mainScale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(mainScale);
		io.ConfigDpiScaleFonts = true;
		io.ConfigDpiScaleViewports = true;

		// ImGui layout save location
		io.IniFilename = nullptr;
		ImGui::LoadIniSettingsFromDisk(s_ImGuiLayoutPath);

		const auto& device = VulkanContext::GetCurrentDevice();
		SwapChainDetails vk_swap_chain_details = RenderCommand::GetSwapChainDetails(
			VulkanContext::GetPhysicalDevice(), VulkanContext::GetCurrentWindow()->GetWindowSurface());

		// Creating ImGui RenderPass
		VkAttachmentDescription attachment{};
		attachment.format = VulkanContext::GetCurrentWindow()->GetSwapChain()->GetSwapChainImageFormat();
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment{};
		color_attachment.attachment = 0;
		color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0; // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = 1;
		info.pAttachments = &attachment;
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.dependencyCount = 1;
		info.pDependencies = &dependency;
		VK_CHECK_RESULT(vkCreateRenderPass(device->GetVulkanDevice(), &info, nullptr, &m_ImGuiLayerRenderPass));

		CreateResources();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(VulkanContext::GetCurrentWindow()->GetGLFWwindow(), true);
		ImGui_ImplVulkan_InitInfo init_info{};
		init_info.Instance = VulkanContext::GetCurrentInstance()->GetVulkanInstance();
		init_info.PhysicalDevice = VulkanContext::GetPhysicalDevice();
		init_info.Device = device->GetVulkanDevice();
		init_info.QueueFamily = device->GetQueueFamilyIndices().GraphicsQueueFamilyIndex;
		init_info.Queue = device->GetGraphicsQueue();
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = VulkanContext::GetCurrentGlobalDescriptorPool()->GetVulkanDescriptorPool();
		init_info.PipelineInfoMain.Subpass = 0;
		init_info.MinImageCount = vk_swap_chain_details.SurfaceCapabilities.minImageCount;
		init_info.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
		init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = VK_NULL_HANDLE;
		init_info.PipelineInfoMain.RenderPass = m_ImGuiLayerRenderPass;
		// init_info.PipelineInfoMain.CheckVkResultFn = vk_check_result;
		ImGui_ImplVulkan_Init(&init_info);
	}

	void ImGuiLayer::OnDestroy() {
		// Saving ImGui Layout
		ImGui::SaveIniSettingsToDisk(s_ImGuiLayoutPath);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		for (uint32_t i = 0; i < m_ImGuiFramebuffers.size(); i++)
			vkDestroyFramebuffer(device, m_ImGuiFramebuffers[i], nullptr);

		vkDestroyRenderPass(device, m_ImGuiLayerRenderPass, nullptr);

		for (auto& imageView : m_ImGuiImageViews) vkDestroyImageView(device, imageView, nullptr);
	}

	void ImGuiLayer::Begin() {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);
	}

	void ImGuiLayer::End() {
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)VulkanContext::GetCurrentWindow()->GetSpecification().Width,
			(float)VulkanContext::GetCurrentWindow()->GetSpecification().Height);

		ImGui::Render();
		ImDrawData* main_draw_data = ImGui::GetDrawData();

		// Begin ImGui Render Pass
		VkClearValue clear_value{};
		clear_value.color = { 0.0f, 0.0f, 0.0f, 1.0f };

		uint32_t imageIndex = VulkanContext::GetCurrentWindow()->GetImageIndex();
		const auto& swapchain = VulkanContext::GetCurrentWindow()->GetSwapChain();

		VkRenderPassBeginInfo imgui_render_pass_begin_info{};
		imgui_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		imgui_render_pass_begin_info.renderPass = m_ImGuiLayerRenderPass;
		imgui_render_pass_begin_info.framebuffer = m_ImGuiFramebuffers[imageIndex];
		imgui_render_pass_begin_info.renderArea.extent = swapchain->GetExtent2D();
		imgui_render_pass_begin_info.clearValueCount = 1;
		imgui_render_pass_begin_info.pClearValues = &clear_value;

		VkCommandBuffer commandBuffer = Renderer::GetActiveVulkanCommandBuffer();
		vkCmdBeginRenderPass(commandBuffer, &imgui_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// Record dear imgui primitives into command buffer
		VulkanContext::GetCurrentDevice()->AccessQueueSafely([&]() {
			ImGui_ImplVulkan_RenderDrawData(main_draw_data, commandBuffer);
		});

		// End ImGui Render Pass
		vkCmdEndRenderPass(commandBuffer);

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();

			// The backend submits and presents to the queue for each platform window
			VulkanContext::GetCurrentDevice()->AccessQueueSafely([]() {
				ImGui::RenderPlatformWindowsDefault();
			});
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiLayer::InvalidateResources() {
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VulkanContext::GetCurrentDevice()->WaitIdle();

		for (uint32_t i = 0; i < m_ImGuiFramebuffers.size(); i++)
			vkDestroyFramebuffer(device, m_ImGuiFramebuffers[i], nullptr);

		for (auto& imageView : m_ImGuiImageViews) vkDestroyImageView(device, imageView, nullptr);

		CreateResources();
	}

	void ImGuiLayer::CreateResources() {
		const auto& device = VulkanContext::GetCurrentDevice();

		const auto& swapchain = VulkanContext::GetCurrentWindow()->GetSwapChain();
		auto imageCount = swapchain->GetImages().size();

		// Image Views creation
		m_ImGuiImageViews.resize(imageCount);

		for (uint32_t i = 0; i < imageCount; i++) {
			VkImageViewCreateInfo vk_image_view_create_info{};
			vk_image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			vk_image_view_create_info.image = swapchain->GetImages()[i];
			vk_image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			vk_image_view_create_info.format = swapchain->GetSwapChainImageFormat();
			vk_image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			vk_image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			vk_image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			vk_image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			vk_image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			vk_image_view_create_info.subresourceRange.baseMipLevel = 0;
			vk_image_view_create_info.subresourceRange.levelCount = 1;
			vk_image_view_create_info.subresourceRange.baseArrayLayer = 0;
			vk_image_view_create_info.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(
				vkCreateImageView(device->GetVulkanDevice(), &vk_image_view_create_info, nullptr, &m_ImGuiImageViews[i]));
		}

		// Creating Framebuffers
		m_ImGuiFramebuffers.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; i++) {
			VkImageView attachment[1] = { m_ImGuiImageViews[i] };
			VkFramebufferCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			info.renderPass = m_ImGuiLayerRenderPass;
			info.attachmentCount = 1;
			info.pAttachments = attachment;
			info.width = swapchain->GetExtent2D().width;
			info.height = swapchain->GetExtent2D().height;
			info.layers = 1;

			VK_CHECK_RESULT(vkCreateFramebuffer(device->GetVulkanDevice(), &info, nullptr, &m_ImGuiFramebuffers[i]));
		}
	}

	void ImGuiLayer::OnEvent(Event& e) {
		// TODO: This currently does nothing
		if (m_BlockEvents) {
			ImGuiIO& io = ImGui::GetIO();
			e.Handled |= e.IsInCategory(EventCategory::MouseEventCategory) && io.WantCaptureMouse;
			e.Handled |= e.IsInCategory(EventCategory::KeyEventCategory) && io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::SetupImGuiStyle() {
		auto& style = ImGui::GetStyle();

		// Sizes
		style.WindowPadding = ImVec2(8, 8);
		style.FramePadding = ImVec2(6, 4);
		style.CellPadding = ImVec2(8, 3);
		style.ItemSpacing = ImVec2(8, 5);
		style.ItemInnerSpacing = ImVec2(6, 4);
		style.IndentSpacing = 16;
		style.ScrollbarSize = 10;
		style.GrabMinSize = 10;

		style.WindowBorderSize = 1;
		style.ChildBorderSize = 1;
		style.PopupBorderSize = 1;
		style.FrameBorderSize = 0;
		style.TabBorderSize = 0;
		style.TabBarBorderSize = 0;
		style.TabBarOverlineSize = 2;
		style.DockingSeparatorSize = 2;

		style.WindowRounding = 6;
		style.ChildRounding = 4;
		style.FrameRounding = 3;
		style.PopupRounding = 5;
		style.ScrollbarRounding = 8;
		style.GrabRounding = 3;
		style.TabRounding = 4;

		style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
		style.WindowMenuButtonPosition = ImGuiDir_None;
		style.ColorButtonPosition = ImGuiDir_Left;
		style.TreeLinesFlags = ImGuiTreeNodeFlags_DrawLinesToNodes;
		style.TreeLinesSize = 1;

		auto& colors = style.Colors;

		auto withAlpha = [](ImVec4 color, float alpha) {
			return ImVec4(color.x, color.y, color.z, alpha);
		};
		auto gray = [](float value) {
			return ImVec4(value / 255.0f, value / 255.0f, value / 255.0f, 1.0f);
		};

		// Text
		colors[ImGuiCol_Text] = Theme::TextColor;
		colors[ImGuiCol_TextDisabled] = Theme::TextDisabled;
		colors[ImGuiCol_TextSelectedBg] = withAlpha(Theme::AccentColor, 0.30f);
		colors[ImGuiCol_TextLink] = Theme::AccentColor;
		colors[ImGuiCol_InputTextCursor] = Theme::TextColor;

		// Windows
		colors[ImGuiCol_WindowBg] = Theme::WindowBg;
		colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
		colors[ImGuiCol_PopupBg] = Theme::PopupBg;
		colors[ImGuiCol_Border] = Theme::WindowBorder;
		colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
		colors[ImGuiCol_MenuBarBg] = Theme::ImGuiTitleBg;
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.55f);

		colors[ImGuiCol_TitleBg] = Theme::ImGuiTitleBg;
		colors[ImGuiCol_TitleBgActive] = Theme::ImGuiTitleBg;
		colors[ImGuiCol_TitleBgCollapsed] = Theme::ImGuiTitleBg;

		// Frames (input fields, checkboxes, sliders)
		colors[ImGuiCol_FrameBg] = Theme::FrameBg;
		colors[ImGuiCol_FrameBgHovered] = Theme::FrameBgHovered;
		colors[ImGuiCol_FrameBgActive] = Theme::FrameBgActive;
		colors[ImGuiCol_CheckMark] = Theme::AccentColor;
		colors[ImGuiCol_CheckboxSelectedBg] = Theme::FrameBg;
		colors[ImGuiCol_SliderGrab] = withAlpha(Theme::AccentColor, 0.80f);
		colors[ImGuiCol_SliderGrabActive] = Theme::AccentColor;

		// Buttons
		colors[ImGuiCol_Button] = Theme::ButtonBg;
		colors[ImGuiCol_ButtonHovered] = Theme::ButtonBgHovered;
		colors[ImGuiCol_ButtonActive] = Theme::ButtonBgActive;

		// Headers (collapsing headers, tree nodes, selectables, menu items)
		colors[ImGuiCol_Header] = Theme::HeaderBg;
		colors[ImGuiCol_HeaderHovered] = Theme::HeaderBgHovered;
		colors[ImGuiCol_HeaderActive] = Theme::HeaderBgActive;
		colors[ImGuiCol_TreeLines] = gray(70);

		// Tabs: the selected tab shares the window's color so that it reads as part of the panel below it
		colors[ImGuiCol_Tab] = Theme::ImGuiTitleBg;
		colors[ImGuiCol_TabHovered] = Theme::HeaderBgHovered;
		colors[ImGuiCol_TabSelected] = Theme::WindowBg;
		colors[ImGuiCol_TabSelectedOverline] = Theme::AccentColor;
		colors[ImGuiCol_TabDimmed] = Theme::ImGuiTitleBg;
		colors[ImGuiCol_TabDimmedSelected] = Theme::WindowBg;
		colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0, 0, 0, 0);

		// Docking
		colors[ImGuiCol_DockingPreview] = withAlpha(Theme::AccentColor, 0.35f);
		colors[ImGuiCol_DockingEmptyBg] = Theme::TitlebarColor;

		// Separators and resize handles stay quiet until hovered
		colors[ImGuiCol_Separator] = Theme::WindowBorder;
		colors[ImGuiCol_SeparatorHovered] = withAlpha(Theme::AccentColor, 0.70f);
		colors[ImGuiCol_SeparatorActive] = Theme::AccentColor;
		colors[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
		colors[ImGuiCol_ResizeGripHovered] = withAlpha(Theme::AccentColor, 0.70f);
		colors[ImGuiCol_ResizeGripActive] = Theme::AccentColor;

		// Scrollbars
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
		colors[ImGuiCol_ScrollbarGrab] = gray(66);
		colors[ImGuiCol_ScrollbarGrabHovered] = gray(86);
		colors[ImGuiCol_ScrollbarGrabActive] = gray(104);

		// Tables
		colors[ImGuiCol_TableHeaderBg] = Theme::HeaderBg;
		colors[ImGuiCol_TableBorderStrong] = Theme::TableBorder;
		colors[ImGuiCol_TableBorderLight] = Theme::TableBorderLight;
		colors[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1, 1, 1, 0.025f);

		// Navigation and drag-drop
		colors[ImGuiCol_NavCursor] = Theme::AccentColor;
		colors[ImGuiCol_NavWindowingHighlight] = withAlpha(Theme::AccentColor, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0, 0, 0, 0.35f);
		colors[ImGuiCol_DragDropTarget] = Theme::AccentColor;
		colors[ImGuiCol_DragDropTargetBg] = withAlpha(Theme::AccentColor, 0.10f);
	}

} // namespace Flameberry
