#pragma once

#include "Vulkan/Pipeline.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/Texture2D.h"

namespace Flameberry {
    struct LineVertex
    {
        glm::vec3 Position{ 0.0f };
        glm::vec3 Color{ 1.0f };
    };

    struct QuadVertex
    {
        glm::vec3 Position{ 0.0f };
        glm::vec3 Color{ 1.0f };
    };

    struct Renderer2DData {
        // Lines
        std::shared_ptr<Pipeline> LinePipeline;
        std::shared_ptr<Buffer> LineVertexBuffer;
        std::vector<LineVertex> LineVertices;

        // Quads
        std::shared_ptr<Pipeline> QuadPipeline;
        std::shared_ptr<Buffer> QuadVertexBuffer, QuadIndexBuffer;
        std::vector<QuadVertex> QuadVertices;

        // Temp
        std::shared_ptr<Texture2D> LightIconTexture;
    };

    class Renderer2D
    {
    public:
        static void Init(const std::shared_ptr<DescriptorSetLayout>& globalDescriptorSetLayout, const std::shared_ptr<RenderPass>& renderPass);
        static void Destroy();

        static void AddGrid(int gridSize);
        static void AddLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
        static void AddBillboard(const glm::vec3& position, float size, const glm::vec3& color, const glm::mat4& viewMatrix);

        static void Render(VkDescriptorSet globalDescriptorSet);
    private:
        static Renderer2DData s_Renderer2DData;
    };
}
