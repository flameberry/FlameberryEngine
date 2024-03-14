#pragma once

#include <thread>
#include <mutex>
#include <functional>
#include <vulkan/vulkan.h>

#include "VulkanContext.h"
#include "Pipeline.h"
#include "StaticMesh.h"
#include "ECS/Components.h"

namespace Flameberry {
    class Renderer
    {
    public:
        using Command = std::function<void(VkCommandBuffer, uint32_t)>;
    public:
        // Initialize and Shutdown the resources of all rendering related classes
        static void Init();
        static void Shutdown();

        // This is a hot function which is most likely a bottleneck and cause of high CPU usage
        static void Submit(const Command&& cmd);
        // Currently just renders, the `Wait` part is for a future implementation of Multi-threading
        static void WaitAndRender();

        // Get the current frame index in the update/main thread
        static uint32_t GetCurrentFrameIndex() { return s_FrameIndex; }
        // Get the current frame index in the render thread
        static uint32_t RT_GetCurrentFrameIndex() { return s_RT_FrameIndex; }
        // The render function to be called in the Render Thread which does the actual rendering by execution of the submitted commands
        static void RT_Render();

        // Rendering Utilities
        static void SubmitMeshWithMaterial(const Ref<StaticMesh>& mesh, const Ref<Pipeline>& pipeline, const MaterialTable& materialTable, const glm::mat4& transform);
        static void RT_BindMesh(VkCommandBuffer cmdBuffer, const Ref<StaticMesh>& mesh);
        static void RT_BindPipeline(VkCommandBuffer cmdBuffer, VkPipeline pipeline);
    private:
        static uint32_t s_RT_FrameIndex, s_FrameIndex;

        // Critical Variables
        static std::vector<Command> s_CommandQueue;
    };
}
