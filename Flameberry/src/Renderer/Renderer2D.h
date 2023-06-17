#pragma once

#include "Vulkan/Pipeline.h"
#include "Vulkan/Buffer.h"

namespace Flameberry {
    struct LineVertex
    {
        glm::vec3 Position{ 0.0f };
        glm::vec3 Color{ 1.0f };
    };

    class Renderer2D
    {
    public:
        static void Init(VkDescriptorSetLayout globalDescriptorSetLayout, const std::shared_ptr<RenderPass>& renderPass);
        static void Destroy();

        static void AddGrid(int gridSize);
        static void AddLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
        static void Render(VkDescriptorSet globalDescriptorSet);
    private:
        // Lines
        static std::shared_ptr<Buffer> s_LineVertexBuffer;
        static std::shared_ptr<Pipeline> s_LinePipeline;
        static VkPipelineLayout s_LinePipelineLayout;
        static std::vector<LineVertex> s_LineVertices;
    };
}