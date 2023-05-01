#pragma once

#include <string>
#include "PerspectiveCamera.h"

#include "Vulkan/VulkanTexture.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanDescriptor.h"
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanMesh.h"

namespace Flameberry {
    class SkyboxRenderer
    {
    public:
        SkyboxRenderer(const std::shared_ptr<VulkanDescriptorPool>& globalDescriptorPool, VkDescriptorSetLayout globalDescriptorLayout, VkRenderPass renderPass);
        ~SkyboxRenderer();

        void OnDraw(
            VkCommandBuffer commandBuffer,
            uint32_t currentFrameIndex,
            VkDescriptorSet globalDescriptorSet,
            const PerspectiveCamera& activeCamera,
            const char* path
        );
        void Load(const char* folderPath);

        void BindCubeMapTextureToUnit(uint32_t unit = 0);

        std::string GetFolderPath() const { return m_FolderPath; }
    private:
        std::string m_FolderPath;

        std::unique_ptr<VulkanBuffer> m_VertexBuffer, m_IndexBuffer;

        std::shared_ptr<VulkanTexture> m_SkyboxTexture;
        std::shared_ptr<VulkanPipeline> m_SkyboxPipeline;
        VkPipelineLayout m_SkyboxPipelineLayout;

        const std::shared_ptr<VulkanDescriptorPool>& m_GlobalDescriptorPool;

        // Test
        VulkanMesh m_CubeMesh;
    };
}
