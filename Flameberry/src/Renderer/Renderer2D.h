#pragma once

#include "Pipeline.h"
#include "Buffer.h"
#include "Texture2D.h"

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

        // TODO: Find a better way to do this
        std::shared_ptr<Texture2D> TextureMap;
    };

    class Renderer2D
    {
    public:
        static void Init(const std::shared_ptr<DescriptorSetLayout>& globalDescriptorSetLayout, const std::shared_ptr<RenderPass>& renderPass);
        static void Destroy();

        static void AddGrid(int gridSize);
        static void AddLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
        static void AddBillboard(const glm::vec3& position, float size, const glm::vec3& color, const glm::mat4& viewMatrix);

        static void BeginScene(VkDescriptorSet globalDescriptorSet);
        static void EndScene();

        static void SetActiveTexture(const std::shared_ptr<Texture2D>& texture) { s_Renderer2DData.TextureMap = texture; }
        static void FlushQuads();
    private:
        static Renderer2DData s_Renderer2DData;

        inline static VkDescriptorSet s_GlobalDescriptorSet = VK_NULL_HANDLE;
    };
}
