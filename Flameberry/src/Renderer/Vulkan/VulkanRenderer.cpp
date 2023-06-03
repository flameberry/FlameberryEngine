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
#include "VulkanTexture.h"
#include "StaticMesh.h"
#include "VulkanDebug.h"

#include "AssetManager/AssetManager.h"

namespace Flameberry {
    VulkanRenderer::VulkanRenderer(VulkanWindow* pWindow, const std::shared_ptr<RenderPass>& shadowMapRenderPass)
    {
        VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
        VkPhysicalDevice physicalDevice = VulkanContext::GetPhysicalDevice();

        m_SwapChain = std::make_unique<VulkanSwapChain>(pWindow->GetWindowSurface());

        VulkanContext::GetCurrentDevice()->AllocateCommandBuffers(m_SwapChain->GetSwapChainImageCount());

        // Create the generic texture descriptor layout
        VulkanTexture::InitStaticResources();
    }

    void VulkanRenderer::WriteMousePickingImagePixelToBuffer(VkBuffer buffer, VkImage image, const glm::vec2& pixelOffset)
    {
        const auto& device = VulkanContext::GetCurrentDevice();

        VkCommandBuffer commandBuffer;
        device->BeginSingleTimeCommandBuffer(commandBuffer);
        {
            VkPipelineStageFlags sourceStageFlags;
            VkPipelineStageFlags destinationStageFlags;

            VkImageMemoryBarrier vk_image_memory_barrier{};

            sourceStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

            vk_image_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vk_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            vk_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            vk_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            vk_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_image_memory_barrier.image = image;
            vk_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vk_image_memory_barrier.subresourceRange.baseMipLevel = 0;
            vk_image_memory_barrier.subresourceRange.levelCount = 1;
            vk_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
            vk_image_memory_barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(commandBuffer, sourceStageFlags, destinationStageFlags, 0, 0, nullptr, 0, nullptr, 1, &vk_image_memory_barrier);
        }

        VkBufferImageCopy region{};
        region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageOffset = { (int)pixelOffset.x, (int)pixelOffset.y };
        region.imageExtent = { 1, 1, 1 };

        region.bufferOffset = 0;
        region.bufferRowLength = 1;
        region.bufferImageHeight = 1;

        vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

        {
            VkPipelineStageFlags sourceStageFlags;
            VkPipelineStageFlags destinationStageFlags;

            VkImageMemoryBarrier vk_image_memory_barrier{};

            sourceStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

            vk_image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vk_image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

            vk_image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            vk_image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            vk_image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            vk_image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vk_image_memory_barrier.image = image;
            vk_image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vk_image_memory_barrier.subresourceRange.baseMipLevel = 0;
            vk_image_memory_barrier.subresourceRange.levelCount = 1;
            vk_image_memory_barrier.subresourceRange.baseArrayLayer = 0;
            vk_image_memory_barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(commandBuffer, sourceStageFlags, destinationStageFlags, 0, 0, nullptr, 0, nullptr, 1, &vk_image_memory_barrier);
        }
        device->EndSingleTimeCommandBuffer(commandBuffer);
    }

    VkCommandBuffer VulkanRenderer::BeginFrame()
    {
        VkResult result = m_SwapChain->AcquireNextImage();
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_SwapChain->Invalidate(m_SwapChain->GetVulkanSwapChain());
            return VK_NULL_HANDLE;
        }
        m_ImageIndex = m_SwapChain->GetAcquiredImageIndex();

        const auto& device = VulkanContext::GetCurrentDevice();
        device->ResetCommandBuffer(m_CurrentFrame);
        device->BeginCommandBuffer(m_CurrentFrame);
        return device->GetCommandBuffer(m_CurrentFrame);
    }

    bool VulkanRenderer::EndFrame()
    {
        bool isResized = false;
        const auto& device = VulkanContext::GetCurrentDevice();
        device->EndCommandBuffer(m_CurrentFrame);

        VkResult queuePresentStatus = m_SwapChain->SubmitCommandBuffer(device->GetCommandBuffer(m_CurrentFrame));
        if (queuePresentStatus == VK_ERROR_OUT_OF_DATE_KHR || queuePresentStatus == VK_SUBOPTIMAL_KHR || VulkanContext::GetCurrentWindow()->IsWindowResized())
        {
            m_SwapChain->Invalidate(m_SwapChain->GetVulkanSwapChain());
            VulkanContext::GetCurrentWindow()->ResetWindowResizedFlag();
            isResized = true;
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
        return isResized;
    }

    VulkanRenderer::~VulkanRenderer()
    {
        VulkanContext::GetCurrentDevice()->WaitIdle();
        VulkanTexture::DestroyStaticResources();
        AssetManager::DestroyAssets();
    }
}
