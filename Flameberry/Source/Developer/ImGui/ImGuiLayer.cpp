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
	ImGuiLayer::ImGuiLayer() {}

	void ImGuiLayer::OnCreate()
	{
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

		ImFontConfig config{};

#ifdef FBY_PLATFORM_MACOS
		constexpr float DPI_SCALE = 2.0f;

		config.OversampleH = 3;
		config.GlyphExtraSpacing = ImVec2(1.4f, 0.62f);
		config.RasterizerMultiply = 1.1f;
		config.OversampleV = 3;
#else
		constexpr float DPI_SCALE = 1.0f;
		config.GlyphExtraSpacing = ImVec2(0.65f, 0.62f);
#endif

		constexpr float smallFontSize = 12.0f * DPI_SCALE;
		constexpr float fontSize = 13.5f * DPI_SCALE;
		constexpr float bigFontSize = 18.0f * DPI_SCALE;

		io.Fonts->AddFontFromFileTTF(FBY_PROJECT_DIR "Flameberry/Assets/Fonts/arial/Arial.ttf", bigFontSize, &config);
		io.FontDefault = io.Fonts->AddFontFromFileTTF(FBY_PROJECT_DIR "Flameberry/Assets/Fonts/arial/Arial.ttf", fontSize, &config);
		io.FontGlobalScale = 1 / DPI_SCALE;

		// Merging Font-Awesome fonts into the default font
		ImFontConfig iconFontConfig;
		iconFontConfig.MergeMode = true;
		iconFontConfig.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
		iconFontConfig.GlyphOffset = ImVec2(0, 5);
		static const ImWchar iconRanges[] = { ICON_MIN_LC, ICON_MAX_LC, 0 };

		io.Fonts->AddFontFromFileTTF(FBY_PROJECT_DIR "Flameberry/Assets/Fonts/lucide/lucide.ttf", fontSize, &iconFontConfig, iconRanges);

		// ImGui layout save location
		io.IniFilename = NULL;
		ImGui::LoadIniSettingsFromDisk(FBY_PROJECT_DIR "Flameberry/Config/Layouts/BaseEditorLayout.ini");

		// Setup Dear ImGui style
		SetupImGuiStyle();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		const auto& device = VulkanContext::GetCurrentDevice();
		SwapChainDetails vk_swap_chain_details = RenderCommand::GetSwapChainDetails(VulkanContext::GetPhysicalDevice(), VulkanContext::GetCurrentWindow()->GetWindowSurface());

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
		init_info.QueueFamily = device->GetQueueFamilyIndices().GraphicsAndComputeSupportedQueueFamilyIndex;
		init_info.Queue = device->GetGraphicsQueue();
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = VulkanContext::GetCurrentGlobalDescriptorPool()->GetVulkanDescriptorPool();
		init_info.Subpass = 0;
		init_info.MinImageCount = vk_swap_chain_details.SurfaceCapabilities.minImageCount;
		init_info.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = VK_NULL_HANDLE;
		init_info.RenderPass = m_ImGuiLayerRenderPass;
		// init_info.CheckVkResultFn = vk_check_result;
		ImGui_ImplVulkan_Init(&init_info);
	}

	void ImGuiLayer::OnDestroy()
	{
		// Saving ImGui Layout
		ImGui::SaveIniSettingsToDisk(FBY_PROJECT_DIR "Flameberry/src/ImGui/imgui.ini");

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		for (uint32_t i = 0; i < m_ImGuiFramebuffers.size(); i++)
			vkDestroyFramebuffer(device, m_ImGuiFramebuffers[i], nullptr);

		vkDestroyRenderPass(device, m_ImGuiLayerRenderPass, nullptr);

		for (auto& imageView : m_ImGuiImageViews)
			vkDestroyImageView(device, imageView, nullptr);
	}

	void ImGuiLayer::Begin()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)VulkanContext::GetCurrentWindow()->GetSpecification().Width, (float)VulkanContext::GetCurrentWindow()->GetSpecification().Height);

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

		uint32_t currentFrameIndex = Renderer::RT_GetCurrentFrameIndex();
		VkCommandBuffer commandBuffer = VulkanContext::GetCurrentDevice()->GetCommandBuffer(currentFrameIndex);
		vkCmdBeginRenderPass(commandBuffer, &imgui_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		// Record dear imgui primitives into command buffer
		ImGui_ImplVulkan_RenderDrawData(main_draw_data, commandBuffer);

		// End ImGui Render Pass
		vkCmdEndRenderPass(commandBuffer);

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void ImGuiLayer::InvalidateResources()
	{
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VulkanContext::GetCurrentDevice()->WaitIdle();

		for (uint32_t i = 0; i < m_ImGuiFramebuffers.size(); i++)
			vkDestroyFramebuffer(device, m_ImGuiFramebuffers[i], nullptr);

		for (auto& imageView : m_ImGuiImageViews)
			vkDestroyImageView(device, imageView, nullptr);

		CreateResources();
	}

	void ImGuiLayer::CreateResources()
	{
		const auto& device = VulkanContext::GetCurrentDevice();

		const auto& swapchain = VulkanContext::GetCurrentWindow()->GetSwapChain();
		auto imageCount = swapchain->GetImages().size();

		// Image Views creation
		m_ImGuiImageViews.resize(imageCount);

		for (uint32_t i = 0; i < imageCount; i++)
		{
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

			VK_CHECK_RESULT(vkCreateImageView(device->GetVulkanDevice(), &vk_image_view_create_info, nullptr, &m_ImGuiImageViews[i]));
		}

		// Creating Framebuffers
		m_ImGuiFramebuffers.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; i++)
		{
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

	void ImGuiLayer::OnEvent(Event& e)
	{
		// TODO: This currently does nothing
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			e.Handled |= e.IsInCategory(EventCategory::MouseEventCategory) && io.WantCaptureMouse;
			e.Handled |= e.IsInCategory(EventCategory::KeyEventCategory) && io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::SetupImGuiStyle()
	{
		auto& style = ImGui::GetStyle();
		style.ColorButtonPosition = ImGuiDir_Left;
		style.WindowRounding = 5;
		style.FrameRounding = 2;
		style.PopupRounding = 3;
		style.CellPadding = ImVec2(8, 7);
		style.ScrollbarSize = 12;
		style.DockingSeparatorSize = 3;
		style.WindowMenuButtonPosition = ImGuiDir_Left;
		style.TabBarBorderSize = 0;

		auto& colors = style.Colors;

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

		// colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
		colors[ImGuiCol_WindowBg] = Theme::WindowBgGrey;
		colors[ImGuiCol_Border] = Theme::WindowBg;

		// Frame
		colors[ImGuiCol_FrameBg] = ImVec4(0.01f, 0.01f, 0.01f, 1.0f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.11f, 0.11f, 0.11f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.08f, 0.08f, 0.08f, 1.0f };

		// colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_TitleBg] = Theme::WindowBg;
		colors[ImGuiCol_TitleBgActive] = colors[ImGuiCol_TitleBg];
		colors[ImGuiCol_TitleBgCollapsed] = colors[ImGuiCol_TitleBg];

		colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 16.0f / 255.0f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.01f, 0.01f, 0.01f, 1.0f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.01f, 0.01f, 0.01f, 1.0f);

		// Button
		colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.0f };

		colors[ImGuiCol_CheckMark] = ImVec4(0.961f, 0.796f, 0.486f, 1.0f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.961f, 0.796f, 0.486f, 1.0f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.961f, 0.796f, 0.486f, 1.0f);

		colors[ImGuiCol_Separator] = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.961f, 0.796f, 0.486f, 1.0f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.961f, 0.796f, 0.486f, 1.0f);

		colors[ImGuiCol_NavHighlight] = ImVec4(0.961f, 0.796f, 0.486f, 1.0f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

		colors[ImGuiCol_DragDropTarget] = ImVec4(0.961f, 0.796f, 0.486f, 1.0f);
	}
} // namespace Flameberry
