#pragma once

#include <thread>
#include <mutex>
#include <functional>
#include <vulkan/vulkan.h>

#include "VulkanContext.h"

namespace Flameberry {
    class Renderer
    {
    public:
        using Command = std::function<void(VkCommandBuffer, uint32_t)>;
    public:
        static void Init();
        static void Shutdown();

        static void Submit(const Command& cmd);
        static void WaitAndRender();
        static uint32_t GetCurrentFrameIndex() { return s_FrameIndex; }

        static uint32_t RT_GetCurrentFrameIndex() { return s_RT_FrameIndex; }
        static void RT_Render();
    private:
        static uint32_t s_RT_FrameIndex, s_FrameIndex;

        // Critical Variables
        static std::vector<Command> s_CommandQueue;
    };
}
