#pragma once

#include <string>
#include "PerspectiveCamera.h"

#include "Vulkan/Texture2D.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/DescriptorSet.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/StaticMesh.h"

namespace Flameberry {
    class SkyboxRenderer
    {
    public:
        SkyboxRenderer(const std::shared_ptr<RenderPass>& renderPass);
        ~SkyboxRenderer();

        void OnDraw(VkDescriptorSet globalDescriptorSet, const PerspectiveCamera& activeCamera);
        std::string GetFolderPath() const { return m_FolderPath; }
    private:
        std::string m_FolderPath;

        std::shared_ptr<Texture2D> m_SkyboxTexture;
        std::shared_ptr<Pipeline> m_SkyboxPipeline;
        VkPipelineLayout m_SkyboxPipelineLayout;
    };
}
