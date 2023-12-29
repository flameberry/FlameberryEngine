#pragma once

#include <memory>
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
        VkCommandBuffer GetVulkanCommandBuffer() const { return m_CommandBuffer; }
    private:
        CommandBufferSpecification m_Specification;

        VkCommandBuffer m_CommandBuffer;
    };

}
