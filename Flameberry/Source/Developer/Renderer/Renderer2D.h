#pragma once

#include "AABB.h"

#include "Pipeline.h"
#include "Buffer.h"
#include "Texture2D.h"
#include "Font.h"

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
		glm::vec2 TextureCoordinates{ 0.0f }; // TODO: Unused
		int EntityIndex = -1;
	};

	struct TextVertex
	{
		glm::vec3 Position{ 0.0f };
		glm::vec3 Color{ 1.0f };
		glm::vec2 TextureCoordinates{ 0.0f };
		int EntityIndex = -1;
	};

	struct Renderer2DData
	{
		// Lines
		Ref<Pipeline> LinePipeline;
		Ref<Buffer> LineVertexBuffer;
		std::vector<LineVertex> LineVertices;

		// Quads
		Ref<Pipeline> QuadPipeline;
		Ref<Buffer> QuadVertexBuffer, QuadIndexBuffer;
		std::vector<QuadVertex> QuadVertices;
		uint32_t QuadVertexBufferOffset = 0;

		Ref<Pipeline> TextPipeline;
		Ref<Buffer> TextVertexBuffer, TextIndexBuffer;
		uint32_t TextVertexBufferOffset = 0;

		struct TextBatch
		{
			Ref<Texture2D> FontAtlasTexture;
			std::vector<TextVertex> TextVertices;
		};

		std::unordered_map<AssetHandle, uint32_t> FontHandleToBatchIndex;
		std::vector<TextBatch> TextBatches;

		// TODO: Find a better way to do this
		Ref<Texture2D> TextureMap;
	};

	struct TextParams
	{
		glm::vec3 Color;
		float Kerning;
		float LineSpacing;
	};

	// To be defined in Components.h
	enum class AxisType : uint8_t;

	class Renderer2D
	{
	public:
		static void Init(const Ref<RenderPass>& renderPass);
		static void Shutdown();

		static void AddLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
		static void AddSemiCircle(const glm::vec3& position, const float radius, const glm::quat& rotation, const glm::vec3& color, int segments = 32);
		static void AddCircle(const glm::vec3& position, const float radius, const glm::quat& rotation, const glm::vec3& color, int segments = 32);
		static void AddBillboard(const glm::vec3& position, float size, const glm::vec3& color, const glm::mat4& viewMatrix, int entityIndex = -1);
		static void AddGrid(int gridSize);
		static void AddAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color);
		static void AddText(const std::string& text, const Ref<Font>& fontAsset, const glm::mat4& transform, const TextParams& textParams, int entityIndex = -1);

		static void BeginScene(VkDescriptorSet globalDescriptorSet);
		static void EndScene();

		static void SetActiveTexture(const Ref<Texture2D>& texture) { s_Renderer2DData.TextureMap = texture; }
		static void FlushQuads();

		static const Renderer2DData& GetRendererData() { return s_Renderer2DData; }

	private:
		static Renderer2DData s_Renderer2DData;

		inline static VkDescriptorSet s_GlobalDescriptorSet = VK_NULL_HANDLE;
	};

} // namespace Flameberry
