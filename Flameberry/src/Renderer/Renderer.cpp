#include "Renderer.h"

#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/VulkanContext.h"
#include "Vulkan/Texture2D.h"

namespace Flameberry {
    std::vector<Renderer::Command> Renderer::s_CommandQueue;
    uint32_t Renderer::s_CurrentFrameIndex = 0;

    void Renderer::Submit(const Command& cmd)
    {
        s_CommandQueue.push_back(cmd);
    }

    void Renderer::Render()
    {
        VkCommandBuffer commandBuffer = VulkanContext::GetCurrentDevice()->GetCommandBuffer(s_CurrentFrameIndex);
        uint32_t imageIndex = VulkanContext::GetCurrentWindow()->GetImageIndex();

        for (auto& cmd : s_CommandQueue)
            cmd(commandBuffer, imageIndex);

        s_CurrentFrameIndex = (s_CurrentFrameIndex + 1) % VulkanSwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::ClearCommandQueue()
    {
        s_CommandQueue.clear();
    }
}