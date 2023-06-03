#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <set>

#include "Core/optional.h"

#include "Renderer/PerspectiveCamera.h"
#include "VulkanVertex.h"
#include "VulkanBuffer.h"
#include "Image.h"
#include "VulkanSwapChain.h"
#include "VulkanContext.h"
#include "Pipeline.h"
#include "VulkanDescriptor.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

namespace Flameberry {
    struct CameraUniformBufferObject { glm::mat4 ViewProjectionMatrix; glm::mat4 LightViewProjectionMatrix; };
    struct ModelMatrixPushConstantData { glm::mat4 ModelMatrix; };
    struct MousePickingPushConstantData { glm::mat4 ModelMatrix; int EntityID; };

    class VulkanRenderer
    {
    public:
        VulkanRenderer(VulkanWindow* pWindow, const std::shared_ptr<RenderPass>& shadowMapRenderPass);
        ~VulkanRenderer();
        [[nodiscard]] VkCommandBuffer BeginFrame();
        bool EndFrame();

        template<typename... Args>
        static std::shared_ptr<VulkanRenderer> Create(Args... args) { return std::make_shared<VulkanRenderer>(std::forward<Args>(args)...); }
    public:
        uint32_t     GetCurrentFrameIndex() const { return m_CurrentFrame; }
        uint32_t     GetImageIndex() const { return m_ImageIndex; }
        VkFormat     GetSwapChainImageFormat() const { return m_SwapChain->GetSwapChainImageFormat(); }
        VkExtent2D   GetSwapChainExtent2D() const { return m_SwapChain->GetExtent2D(); }
        std::vector<VkImageView> GetSwapChainImageViews() const { return m_SwapChain->GetImageViews(); }
        std::vector<VkImage>     GetSwapChainImages() const { return m_SwapChain->GetImages(); }

        void WriteMousePickingImagePixelToBuffer(VkBuffer buffer, VkImage image, const glm::vec2& pixelOffset = { 0, 0 });
    private:
        uint32_t m_CurrentFrame = 0;
        uint32_t m_ImageIndex;
        bool m_EnableShadows = true;

        std::unique_ptr<VulkanSwapChain> m_SwapChain;
    };
}
