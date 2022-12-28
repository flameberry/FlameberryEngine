#include "VulkanRenderer.h"

#include <cstdint>
#include <algorithm>
#include <chrono>
#include <thread>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Core/Core.h"
#include "Core/Timer.h"
#include "VulkanRenderCommand.h"

namespace Flameberry {
    VulkanRenderer::VulkanRenderer(VulkanWindow* pWindow)
        : m_VulkanContext(pWindow)
    {
        VulkanContext::SetCurrentContext(&m_VulkanContext);

        VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VkPhysicalDevice physicalDevice = VulkanContext::GetPhysicalDevice();

        m_SwapChain = std::make_unique<VulkanSwapChain>(pWindow->GetWindowSurface());

        VulkanContext::GetCurrentDevice()->AllocateCommandBuffers(m_SwapChain->GetSwapChainImageCount());
    }

    VkCommandBuffer VulkanRenderer::BeginFrame()
    {
        VkResult result = m_SwapChain->AcquireNextImage();
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_SwapChain->Invalidate();
            return VK_NULL_HANDLE;
        }
        m_ImageIndex = m_SwapChain->GetAcquiredImageIndex();

        const auto& device = VulkanContext::GetCurrentDevice();
        device->ResetCommandBuffer(m_CurrentFrame);
        device->BeginCommandBuffer(m_CurrentFrame);
        return device->GetCommandBuffer(m_CurrentFrame);
    }

    void VulkanRenderer::EndFrame()
    {
        const auto& device = VulkanContext::GetCurrentDevice();
        device->EndCommandBuffer(m_CurrentFrame);

        VkResult queuePresentStatus = m_SwapChain->SubmitCommandBuffer(device->GetCommandBuffer(m_CurrentFrame));
        if (queuePresentStatus == VK_ERROR_OUT_OF_DATE_KHR || queuePresentStatus == VK_SUBOPTIMAL_KHR || VulkanContext::GetCurrentWindow()->IsWindowResized())
        {
            m_SwapChain->Invalidate(m_SwapChain->GetVulkanSwapChain());
            VulkanContext::GetCurrentWindow()->ResetWindowResizedFlag();
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanRenderer::BeginRenderPass()
    {
        VkRenderPassBeginInfo vk_render_pass_begin_info{};
        vk_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vk_render_pass_begin_info.renderPass = m_SwapChain->GetRenderPass();
        vk_render_pass_begin_info.framebuffer = m_SwapChain->GetFramebuffer(m_ImageIndex);
        vk_render_pass_begin_info.renderArea.offset = { 0, 0 };
        vk_render_pass_begin_info.renderArea.extent = m_SwapChain->GetExtent2D();

        std::array<VkClearValue, 2> vk_clear_values{};
        vk_clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        vk_clear_values[1].depthStencil = { 1.0f, 0 };

        vk_render_pass_begin_info.clearValueCount = static_cast<uint32_t>(vk_clear_values.size());
        vk_render_pass_begin_info.pClearValues = vk_clear_values.data();

        const auto& device = VulkanContext::GetCurrentDevice();
        vkCmdBeginRenderPass(device->GetCommandBuffer(m_CurrentFrame), &vk_render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderer::EndRenderPass()
    {
        const auto& device = VulkanContext::GetCurrentDevice();
        vkCmdEndRenderPass(device->GetCommandBuffer(m_CurrentFrame));
    }

    VulkanRenderer::~VulkanRenderer()
    {
        VulkanContext::GetCurrentDevice()->WaitIdle();
    }
}
