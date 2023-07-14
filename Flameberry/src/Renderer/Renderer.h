#pragma once

#include <functional>
#include <vulkan/vulkan.h>

#include "VulkanContext.h"

namespace Flameberry {
    class Renderer
    {
    public:
        using Command = std::function<void(VkCommandBuffer, uint32_t)>;
    public:
        static void Submit(const Command& cmd);
        static void Render();
        static void ClearCommandQueue();

        static uint32_t GetCurrentFrameIndex() { return VulkanContext::GetCurrentWindow()->GetCurrentFrameIndex(); }
    private:
        static uint32_t s_CurrentFrameIndex;
        static std::vector<Command> s_CommandQueue;
    };
}