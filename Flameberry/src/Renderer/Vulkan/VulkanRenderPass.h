// #pragma once

// #include <memory>
// #include <vulkan/vulkan.h>

// namespace Flameberry {
//     class VulkanRenderPass
//     {
//     public:
//         VulkanRenderPass();
//         ~VulkanRenderPass();

//         template<typename... Args>
//         static std::shared_ptr<VulkanRenderPass> Create(Args... args) { return std::make_shared<VulkanRenderPass>(std::forward<Args>(args)...); }

//         VkRenderPass GetVulkanRenderPass() const { return m_VkRenderPass; }
//     private:
//         VkRenderPass m_VkRenderPass;
//     };
// }