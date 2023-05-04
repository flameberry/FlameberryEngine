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
#include "VulkanPipeline.h"
#include "VulkanDescriptor.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

namespace Flameberry {
    struct CameraUniformBufferObject { glm::mat4 ViewProjectionMatrix; glm::mat4 LightViewProjectionMatrix; };
    struct ModelMatrixPushConstantData { glm::mat4 ModelMatrix; };

    class VulkanRenderer
    {
    public:
        VulkanRenderer(VulkanWindow* pWindow);
        ~VulkanRenderer();
        [[nodiscard]] VkCommandBuffer BeginFrame();
        bool EndFrame();
        void BeginViewportRenderPass(const glm::vec3& clearColor = glm::vec3(0));
        void EndViewportRenderPass();

        void BeginShadowRenderPass(const glm::mat4& lightViewProjectionMatrix);
        void EndShadowRenderPass();

        template<typename... Args>
        static std::shared_ptr<VulkanRenderer> Create(Args... args) { return std::make_shared<VulkanRenderer>(std::forward<Args>(args)...); }
    public:
        VkRenderPass GetRenderPass() const { return m_ViewportRenderPass; }
        uint32_t     GetCurrentFrameIndex() const { return m_CurrentFrame; }
        uint32_t     GetImageIndex() const { return m_ImageIndex; }
        VkFormat     GetSwapChainImageFormat() const { return m_SwapChain->GetSwapChainImageFormat(); }
        VkExtent2D   GetSwapChainExtent2D() const { return m_SwapChain->GetExtent2D(); }
        std::vector<VkImageView> GetSwapChainImageViews() const { return m_SwapChain->GetImageViews(); }
        std::vector<VkImage>     GetSwapChainImages() const { return m_SwapChain->GetImages(); }

        VkImageView  GetViewportImageView(uint32_t index) { return m_ViewportImages[index]->GetImageView(); }

        // Shadow Map
        VkPipelineLayout GetShadowMapPipelineLayout() const { return m_ShadowMapPipelineLayout; }
        VkImageView GetShadowMapImageView(uint32_t index) const { return m_ShadowMapImages[index]->GetImageView(); }
        VkSampler GetShadowMapSampler(uint32_t index) const { return m_ShadowMapSamplers[index]; }
        bool EnableShadows() const { return m_EnableShadows; }

        // Viewport Rendering
        void CreateViewportResources();
        void CreateViewportRenderPass();
        void InvalidateViewportResources();
        void UpdateViewportSize(const glm::vec2& viewportSize);
    private:
        uint32_t m_CurrentFrame = 0;
        uint32_t m_ImageIndex;
        bool m_EnableShadows = true;

        VulkanContext m_VulkanContext;
        std::unique_ptr<VulkanSwapChain> m_SwapChain;

        // Shadow Resources
        std::vector<std::shared_ptr<VulkanImage>> m_ShadowMapImages;
        std::vector<VkSampler> m_ShadowMapSamplers;
        std::vector<VkFramebuffer> m_ShadowMapFramebuffers;

        std::unique_ptr<VulkanPipeline> m_ShadowMapPipeline;
        VkPipelineLayout m_ShadowMapPipelineLayout;
        VkRenderPass m_ShadowMapRenderPass;

        std::unique_ptr<VulkanDescriptorLayout> m_ShadowMapDescriptorLayout;
        std::unique_ptr<VulkanDescriptorWriter> m_ShadowMapDescriptorWriter;
        std::vector<VkDescriptorSet> m_ShadowMapDescriptorSets;
        std::vector<std::unique_ptr<VulkanBuffer>> m_ShadowMapUniformBuffers;

        const uint32_t SHADOW_MAP_WIDTH = 2048, SHADOW_MAP_HEIGHT = 2048;

        // offscreen rendering resources
        std::vector<std::shared_ptr<VulkanImage>> m_ViewportImagesMSAA, m_ViewportImages;
        std::shared_ptr<VulkanImage> m_ViewportDepthImage;
        std::vector<VkFramebuffer> m_ViewportFramebuffers;
        VkRenderPass m_ViewportRenderPass;
        glm::vec2 m_ViewportSize{1280, 720};
    };
}
