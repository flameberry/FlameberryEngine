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
        VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }
        
        template <typename ...Args>
        static std::shared_ptr<CommandBuffer> Create(Args... args) { return std::make_shared<CommandBuffer>(std::forward<Args>(args)...); }
    private:
        CommandBufferSpecification m_CommandBufferSpec;

        VkCommandBuffer m_CommandBuffer;
    };

}
