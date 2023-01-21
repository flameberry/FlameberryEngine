#include "ImGuiLayer.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "Renderer/Vulkan/VulkanContext.h"
#include "Renderer/Vulkan/VulkanRenderCommand.h"
#include "Renderer/Vulkan/VulkanSwapChain.h"
#include "Renderer/Vulkan/VulkanDebug.h"

namespace Flameberry {
    void ImGuiLayer::OnAttach(VkDescriptorPool descriptorPool, VkFormat swapChainImageFormat, const std::vector<VkImageView>& imageViews, VkExtent2D extent)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        const auto& device = VulkanContext::GetCurrentDevice();
        SwapChainDetails vk_swap_chain_details = VulkanRenderCommand::GetSwapChainDetails(VulkanContext::GetPhysicalDevice(), VulkanContext::GetCurrentWindow()->GetWindowSurface());

        // Creating ImGui RenderPass
        VkAttachmentDescription attachment{};
        attachment.format = swapChainImageFormat;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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
        dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
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

        // Creating Framebuffers
        m_ImGuiFramebuffers.resize(VulkanSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (uint32_t i = 0; i < VulkanSwapChain::MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkImageView attachment[1] = { imageViews[i] };
            VkFramebufferCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            info.renderPass = m_ImGuiLayerRenderPass;
            info.attachmentCount = 1;
            info.pAttachments = attachment;
            info.width = extent.width;
            info.height = extent.height;
            info.layers = 1;

            VK_CHECK_RESULT(vkCreateFramebuffer(device->GetVulkanDevice(), &info, nullptr, &m_ImGuiFramebuffers[i]));
        }

        SetupImGuiStyle();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(VulkanContext::GetCurrentWindow()->GetGLFWwindow(), true);
        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance = VulkanContext::GetCurrentInstance()->GetVulkanInstance();
        init_info.PhysicalDevice = VulkanContext::GetPhysicalDevice();
        init_info.Device = device->GetVulkanDevice();
        init_info.QueueFamily = device->GetQueueFamilyIndices().GraphicsSupportedQueueFamilyIndex;
        init_info.Queue = device->GetGraphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = descriptorPool;
        init_info.Subpass = 0;
        init_info.MinImageCount = vk_swap_chain_details.SurfaceCapabilities.minImageCount;
        init_info.ImageCount = VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = VK_NULL_HANDLE;
        // init_info.CheckVkResultFn = vk_check_result;
        ImGui_ImplVulkan_Init(&init_info, m_ImGuiLayerRenderPass);

        // Upload fonts
        VkCommandBuffer commandBuffer;
        device->BeginSingleTimeCommandBuffer(commandBuffer);
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        device->EndSingleTimeCommandBuffer(commandBuffer);
        device->WaitIdle();

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void ImGuiLayer::OnDetach()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        for (uint32_t i = 0; i < m_ImGuiFramebuffers.size(); i++)
            vkDestroyFramebuffer(device, m_ImGuiFramebuffers[i], nullptr);
        vkDestroyRenderPass(device, m_ImGuiLayerRenderPass, nullptr);
    }

    void ImGuiLayer::Begin()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    }

    void ImGuiLayer::End(VkCommandBuffer commandBuffer, uint32_t currentFrameIndex, VkExtent2D extent)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)VulkanContext::GetCurrentWindow()->GetWidth(), (float)VulkanContext::GetCurrentWindow()->GetHeight());

        ImGui::Render();
        ImDrawData* main_draw_data = ImGui::GetDrawData();

        // Begin ImGui Render Pass
        VkClearValue clear_value{};
        clear_value.color = { 0.0f };

        VkRenderPassBeginInfo imgui_render_pass_begin_info{};
        imgui_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        imgui_render_pass_begin_info.renderPass = m_ImGuiLayerRenderPass;
        imgui_render_pass_begin_info.framebuffer = m_ImGuiFramebuffers[currentFrameIndex];
        imgui_render_pass_begin_info.renderArea.extent.width = extent.width;
        imgui_render_pass_begin_info.renderArea.extent.height = extent.height;
        imgui_render_pass_begin_info.clearValueCount = 1;
        imgui_render_pass_begin_info.pClearValues = &clear_value;
        vkCmdBeginRenderPass(commandBuffer, &imgui_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(main_draw_data, commandBuffer);

        // End ImGui Render Pass
        vkCmdEndRenderPass(commandBuffer);

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void ImGuiLayer::SetupImGuiStyle()
    {
        auto& colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

        // Headers
        colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Buttons
        colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
        colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
        colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
        colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
        colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

        // Title
        colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    }
}
