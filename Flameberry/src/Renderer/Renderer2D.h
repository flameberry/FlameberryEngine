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
        int EntityIndex = -1;
    };

    struct Renderer2DData {
        // Lines
        Ref<Pipeline> LinePipeline;
        Ref<Buffer> LineVertexBuffer;
        std::vector<LineVertex> LineVertices;

        // Quads
        Ref<Pipeline> QuadPipeline;
        Ref<Buffer> QuadVertexBuffer, QuadIndexBuffer;
        std::vector<QuadVertex> QuadVertices;
        uint32_t VertexBufferOffset = 0;

        // TODO: Find a better way to do this
        Ref<Texture2D> TextureMap;
    };

    class Renderer2D
    {
    public:
        static void Init(const Ref<RenderPass>& renderPass);
        static void Shutdown();

        static void AddGrid(int gridSize);
        static void AddLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
        static void AddBillboard(const glm::vec3& position, float size, const glm::vec3& color, const glm::mat4& viewMatrix, int entityIndex = -1);

        static void BeginScene(VkDescriptorSet globalDescriptorSet);
        static void EndScene();

        static void SetActiveTexture(const Ref<Texture2D>& texture) { s_Renderer2DData.TextureMap = texture; }
        static void FlushQuads();

        static const Renderer2DData& GetRendererData() { return s_Renderer2DData; }
    private:
        static Renderer2DData s_Renderer2DData;

        inline static VkDescriptorSet s_GlobalDescriptorSet = VK_NULL_HANDLE;
    };
}
