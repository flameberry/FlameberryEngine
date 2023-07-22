#pragma once

#include <vulkan/vulkan.h>

namespace Flameberry {

    struct CommandBufferSpecification {
        VkCommandPool CommandPool;
        bool IsPrimary = true, SingleTimeUsage = false;
    };

    class CommandBuffer
    {
    public:
        CommandBuffer(const CommandBufferSpecification& specification);
        ~CommandBuffer();

        void Begin();
        void End();

        void Reset();
        VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }
    private:
        CommandBufferSpecification m_CommandBufferSpec;

        VkCommandBuffer m_CommandBuffer;
    };

}