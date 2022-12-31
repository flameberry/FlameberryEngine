#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <set>

#include "Core/optional.h"

#include "Renderer/PerspectiveCamera.h"
#include "VulkanVertex.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanSwapChain.h"
#include "VulkanContext.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

namespace Flameberry {
    struct CameraUniformBufferObject { glm::mat4 ViewProjectionMatrix; };
    struct ModelMatrixPushConstantData { glm::mat4 ModelMatrix; };

    class VulkanRenderer
    {
    public:
        VulkanRenderer(VulkanWindow* pWindow);
        ~VulkanRenderer();
        [[nodiscard]] VkCommandBuffer BeginFrame();
        void EndFrame();
        void BeginRenderPass();
        void EndRenderPass();

        template<typename... Args>
        static std::shared_ptr<VulkanRenderer> Create(Args... args) { return std::make_shared<VulkanRenderer>(std::forward<Args>(args)...); }
    public:
        // VkDevice& GetDevice() { return m_VkDevice; }
        // VkQueue GetGraphicsQueue() { return m_VkGraphicsQueue; }
        // VkQueue GetPresentationQueue() { return m_VkPresentationQueue; }
        // VkSurfaceKHR GetSurface() { return m_VkSurface; }
        VkRenderPass GetRenderPass() { return m_SwapChain->GetRenderPass(); }
        uint32_t GetCurrentFrameIndex() { return m_CurrentFrame; }
        VkFormat GetSwapChainImageFormat() { return m_SwapChain->GetSwapChainImageFormat(); }
        // GLFWwindow* GetUserGLFWwindow() { return m_UserGLFWwindow; }
        // const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() {
        //     VkPhysicalDeviceProperties properties{};
        //     vkGetPhysicalDeviceProperties(m_VkPhysicalDevice, &properties);
        //     return properties;
        // }

        void       CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);
        VkExtent2D GetSwapChainExtent2D() { return m_SwapChain->GetExtent2D(); };
    private:
        uint32_t m_CurrentFrame = 0;
        uint32_t m_ImageIndex;

        VulkanContext m_VulkanContext;
        std::unique_ptr<VulkanSwapChain> m_SwapChain;
    };
}
