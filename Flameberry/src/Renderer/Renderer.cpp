#include "Renderer.h"

#include "Core/Application.h"
#include "Core/Profiler.h"

#include "VulkanContext.h"
#include "SwapChain.h"
#include "Texture2D.h"
#include "Material.h"

namespace Flameberry {
    std::vector<Renderer::Command> Renderer::s_CommandQueue;
    uint32_t Renderer::s_RT_FrameIndex = 0, Renderer::s_FrameIndex = 0;

    void Renderer::Init()
    {
        // Create the generic texture descriptor layout
        Texture2D::InitStaticResources();
        Material::Init();
    }

    void Renderer::Shutdown()
    {
        Material::Shutdown();
        Texture2D::DestroyStaticResources();
    }

    void Renderer::Submit(const Command& cmd)
    {
        s_CommandQueue.push_back(cmd);
    }

    void Renderer::WaitAndRender()
    {
        // Update the Frame Index of the Main Thread
        s_FrameIndex = (s_FrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
        RT_Render();
    }

    void Renderer::RT_Render()
    {
        FBY_PROFILE_SCOPE("RT_RenderLoop");
        auto& window = Application::Get().GetWindow();
        const auto& device = VulkanContext::GetCurrentDevice();

        // Execute all Render Commands
        if (window.BeginFrame())
        {
            device->ResetCommandBuffer(s_RT_FrameIndex);
            device->BeginCommandBuffer(s_RT_FrameIndex);

            VkCommandBuffer commandBuffer = VulkanContext::GetCurrentDevice()->GetCommandBuffer(s_RT_FrameIndex);
            uint32_t imageIndex = window.GetImageIndex();

            int i = 0;
            for (auto& cmd : s_CommandQueue)
            {
                cmd(commandBuffer, imageIndex);
                i++;
            }

            device->EndCommandBuffer(s_RT_FrameIndex);
            window.SwapBuffers();
        }

        s_CommandQueue.clear();

        // Update the Frame Index of the Render Thread
        s_RT_FrameIndex = (s_RT_FrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
    }
}
