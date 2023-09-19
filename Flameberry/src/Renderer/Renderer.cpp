#include "Renderer.h"

#include "Core/Application.h"
#include "Core/Profiler.h"

#include "SwapChain.h"
#include "Texture2D.h"
#include "VulkanContext.h"

namespace Flameberry {
    // THE RENDER THREAD
    // std::thread Renderer::s_RenderThread;

    std::vector<Renderer::Command> Renderer::s_CommandQueues[2];
    uint32_t Renderer::s_RT_FrameIndex = 0, Renderer::s_FrameIndex = 0;

    // Critical Variables
    std::mutex Renderer::s_UpdateReadyMutex, Renderer::s_RenderFinishedMutex;
    std::condition_variable Renderer::s_UpdateCV, Renderer::s_RenderCV;
    std::atomic<bool> Renderer::s_UpdateReady = false, Renderer::s_RenderFinished = true;

    void Renderer::Init()
    {
        // s_RenderThread = std::thread(Renderer::RT_Render);
    }

    void Renderer::Shutdown()
    {
        // s_RenderThread.join();
    }

    void Renderer::Submit(const Command& cmd)
    {
        // s_CommandQueues[!s_ExecutingCmdQueueIndex].push_back(cmd);
        s_CommandQueues[0].push_back(cmd);
    }

    void Renderer::WaitAndRender()
    {
        // FL_LOG("Main Thread: {0}", s_FrameIndex);

        // Update the Frame Index of the Main Thread
        s_FrameIndex = (s_FrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;

        // Signal Frame Update Ready Semaphore
        // std::lock_guard lg(s_UpdateReadyMutex);
        // s_UpdateReady = true;
        // s_ExecutingCmdQueueIndex = !s_ExecutingCmdQueueIndex;
        // s_UpdateCV.notify_one();

        // Wait for Render Frame Finished Semaphore
        // std::unique_lock ul(s_RenderFinishedMutex);
        // s_RenderCV.wait(ul, [=]() { return s_RenderFinished.load(); });

        RT_Render();
    }

    void Renderer::RT_Render()
    {
        FL_PROFILE_SCOPE("RT_RenderLoop");
        auto& window = Application::Get().GetWindow();
        const auto& device = VulkanContext::GetCurrentDevice();

        // Check for Frame Update Ready Semaphore
        // std::unique_lock ul(s_UpdateReadyMutex);
        // s_UpdateCV.wait(ul, [=]() { return s_UpdateReady.load(); });
        // s_RenderFinished = false;

        // Execute all Render Commands
        if (window.BeginFrame())
        {
            device->ResetCommandBuffer(s_RT_FrameIndex);
            device->BeginCommandBuffer(s_RT_FrameIndex);

            VkCommandBuffer commandBuffer = VulkanContext::GetCurrentDevice()->GetCommandBuffer(s_RT_FrameIndex);
            uint32_t imageIndex = window.GetImageIndex();

            for (auto& cmd : s_CommandQueues[0 /*s_ExecutingCmdQueueIndex*/])
                cmd(commandBuffer, imageIndex);

            device->EndCommandBuffer(s_RT_FrameIndex);
            window.SwapBuffers();
        }

        s_CommandQueues[0].clear();

        // FL_LOG("Render Thread: {0}", s_RT_FrameIndex);

        // Update the Frame Index of the Render Thread
        s_RT_FrameIndex = (s_RT_FrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;

        // Signal Render Frame Finished Semaphore
        // std::lock_guard lg(s_RenderFinishedMutex);
        // s_RenderFinished = true;

        // TODO: This is questionable
        // s_UpdateReady = false;

        // s_CommandQueues[s_ExecutingCmdQueueIndex].clear();
        // s_RenderCV.notify_one();
    }
}
