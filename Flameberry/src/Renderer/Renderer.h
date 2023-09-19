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
        // THE RENDER THREAD
        static std::thread s_RenderThread;

        static uint32_t s_RT_FrameIndex, s_FrameIndex;

        // Critical Variables
        static std::vector<Command> s_CommandQueues[2];
        static inline std::atomic<uint8_t> s_ExecutingCmdQueueIndex = 0;

        // Synchronisation
        static std::mutex s_UpdateReadyMutex, s_RenderFinishedMutex;
        static std::condition_variable s_UpdateCV, s_RenderCV;
        static std::atomic<bool> s_UpdateReady, s_RenderFinished;
    };
}