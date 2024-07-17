#include "Renderer2D.h"

#include "ECS/ecs.hpp"
#include "Renderer.h"

#include "ShaderLibrary.h"
#include "RenderPass.h"
#include "VulkanDebug.h"
#include "RenderCommand.h"
#include "MSDFFontData.h"

#define MAX_LINES 10000
#define MAX_QUADS 10000
#define MAX_QUAD_VERTICES MAX_QUADS * 4
#define MAX_QUAD_INDICES MAX_QUADS * 6

namespace Flameberry {

	Renderer2DData Renderer2D::s_Renderer2DData;

	void Renderer2D::Init(const Ref<RenderPass>& renderPass)
	{
		const auto& device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		{
			// Line Resources
			BufferSpecification bufferSpec{};
			bufferSpec.InstanceCount = 1;
			bufferSpec.InstanceSize = MAX_LINES * 2 * sizeof(LineVertex);
			bufferSpec.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			bufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			s_Renderer2DData.LineVertexBuffer = CreateRef<Buffer>(bufferSpec);
			s_Renderer2DData.LineVertexBuffer->MapMemory(bufferSpec.InstanceSize);

			PipelineSpecification pipelineSpec{};
			pipelineSpec.Shader = ShaderLibrary::Get("SolidColor");
			pipelineSpec.RenderPass = renderPass;

			pipelineSpec.VertexLayout = {
				ShaderDataType::Float3, // a_Position
				ShaderDataType::Float3	// a_Color

			};
			pipelineSpec.Samples = RenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());

			pipelineSpec.PolygonMode = VK_POLYGON_MODE_LINE;
			pipelineSpec.PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			pipelineSpec.CullMode = VK_CULL_MODE_NONE;

			s_Renderer2DData.LinePipeline = CreateRef<Pipeline>(pipelineSpec);
		}

		{
			// Quad Resources
			BufferSpecification bufferSpec{};
			bufferSpec.InstanceCount = 1;
			bufferSpec.InstanceSize = MAX_QUAD_VERTICES * sizeof(QuadVertex);
			bufferSpec.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			bufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			s_Renderer2DData.QuadVertexBuffer = CreateRef<Buffer>(bufferSpec);
			s_Renderer2DData.QuadVertexBuffer->MapMemory(bufferSpec.InstanceSize);

			BufferSpecification indexBufferSpec{};
			indexBufferSpec.InstanceCount = 1;
			indexBufferSpec.InstanceSize = MAX_QUAD_INDICES * sizeof(uint32_t);
			indexBufferSpec.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			indexBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			s_Renderer2DData.QuadIndexBuffer = CreateRef<Buffer>(indexBufferSpec);

			uint32_t* indices = new uint32_t[MAX_QUAD_INDICES];
			uint32_t offset = 0;
			for (uint32_t i = 0; i < MAX_QUAD_INDICES; i += 6)
			{
				indices[i + 0] = offset + 0;
				indices[i + 1] = offset + 1;
				indices[i + 2] = offset + 2;

				indices[i + 3] = offset + 2;
				indices[i + 4] = offset + 3;
				indices[i + 5] = offset + 0;

				offset += 4;
			}

			BufferSpecification stagingBufferSpec{};
			stagingBufferSpec.InstanceCount = 1;
			stagingBufferSpec.InstanceSize = indexBufferSpec.InstanceSize;
			stagingBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			Buffer stagingBuffer(stagingBufferSpec);

			stagingBuffer.MapMemory(indexBufferSpec.InstanceSize);
			stagingBuffer.WriteToBuffer(indices, indexBufferSpec.InstanceSize, 0);
			stagingBuffer.UnmapMemory();

			RenderCommand::CopyBuffer(stagingBuffer.GetVulkanBuffer(), s_Renderer2DData.QuadIndexBuffer->GetVulkanBuffer(), indexBufferSpec.InstanceSize);

			delete[] indices;

			PipelineSpecification pipelineSpec{};
			pipelineSpec.Shader = ShaderLibrary::Get("Quad");
			pipelineSpec.RenderPass = renderPass;

			pipelineSpec.VertexLayout = {
				ShaderDataType::Float3, // a_Position
				ShaderDataType::Float3, // a_Color
				ShaderDataType::Dummy8, // Texture Coordinates (Currently Unnecessary)
				ShaderDataType::Dummy4	// EntityIndex (Unnecessary)
			};

			pipelineSpec.Samples = RenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());
			pipelineSpec.BlendingEnable = true;

			pipelineSpec.CullMode = VK_CULL_MODE_FRONT_BIT;

			s_Renderer2DData.QuadPipeline = CreateRef<Pipeline>(pipelineSpec);
		}

		{
			// Text Resources
			BufferSpecification bufferSpec{};
			bufferSpec.InstanceCount = 1;
			bufferSpec.InstanceSize = MAX_QUAD_VERTICES * sizeof(TextVertex);
			bufferSpec.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			bufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			s_Renderer2DData.TextVertexBuffer = CreateRef<Buffer>(bufferSpec);
			s_Renderer2DData.TextVertexBuffer->MapMemory(bufferSpec.InstanceSize);

			BufferSpecification indexBufferSpec{};
			indexBufferSpec.InstanceCount = 1;
			indexBufferSpec.InstanceSize = MAX_QUAD_INDICES * sizeof(uint32_t);
			indexBufferSpec.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			indexBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			s_Renderer2DData.TextIndexBuffer = CreateRef<Buffer>(indexBufferSpec);

			uint32_t* indices = new uint32_t[MAX_QUAD_INDICES];
			uint32_t offset = 0;
			for (uint32_t i = 0; i < MAX_QUAD_INDICES; i += 6)
			{
				indices[i + 0] = offset + 0;
				indices[i + 1] = offset + 1;
				indices[i + 2] = offset + 2;

				indices[i + 3] = offset + 2;
				indices[i + 4] = offset + 3;
				indices[i + 5] = offset + 0;

				offset += 4;
			}

			BufferSpecification stagingBufferSpec{};
			stagingBufferSpec.InstanceCount = 1;
			stagingBufferSpec.InstanceSize = indexBufferSpec.InstanceSize;
			stagingBufferSpec.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingBufferSpec.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			Buffer stagingBuffer(stagingBufferSpec);

			stagingBuffer.MapMemory(indexBufferSpec.InstanceSize);
			stagingBuffer.WriteToBuffer(indices, indexBufferSpec.InstanceSize, 0);
			stagingBuffer.UnmapMemory();

			RenderCommand::CopyBuffer(stagingBuffer.GetVulkanBuffer(), s_Renderer2DData.TextIndexBuffer->GetVulkanBuffer(), indexBufferSpec.InstanceSize);

			delete[] indices;

			PipelineSpecification pipelineSpec{};
			pipelineSpec.Shader = ShaderLibrary::Get("Text");
			pipelineSpec.RenderPass = renderPass;

			pipelineSpec.VertexLayout = {
				ShaderDataType::Float3, // a_Position
				ShaderDataType::Float3, // a_Color
				ShaderDataType::Float2, // a_TextureCoords
				ShaderDataType::Dummy4	// EntityIndex (Unnecessary)
			};

			pipelineSpec.Samples = RenderCommand::GetMaxUsableSampleCount(VulkanContext::GetPhysicalDevice());
			pipelineSpec.BlendingEnable = true;

			pipelineSpec.CullMode = VK_CULL_MODE_NONE;

			s_Renderer2DData.TextPipeline = CreateRef<Pipeline>(pipelineSpec);
		}
	}

	void Renderer2D::AddLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color)
	{
		s_Renderer2DData.LineVertices.push_back(LineVertex{ start, color });
		s_Renderer2DData.LineVertices.push_back(LineVertex{ end, color });
	}

	void Renderer2D::AddSemiCircle(const glm::vec3& position, const float radius, const glm::quat& rotation, const glm::vec3& color, int segments)
	{
		constexpr float PI = glm::pi<float>();
		const float angle = PI / segments;

		glm::vec3 lastVertex = position + rotation * glm::vec3(radius, 0.0f, 0.0f);
		for (int i = 1; i <= segments; i++)
		{
			glm::vec3 vertex = position + rotation * glm::vec3(radius * cos(i * angle), 0.0f, radius * sin(i * angle));
			Renderer2D::AddLine(lastVertex, vertex, color);
			lastVertex = vertex;
		}
	}

	void Renderer2D::AddCircle(const glm::vec3& position, const float radius, const glm::quat& rotation, const glm::vec3& color, int segments)
	{
		constexpr float PI = glm::pi<float>();
		const float angle = 2 * PI / segments;

		glm::vec3 lastVertex = position + rotation * glm::vec3(radius, 0.0f, 0.0f);
		for (int i = 1; i <= segments; i++)
		{
			glm::vec3 vertex = position + rotation * glm::vec3(radius * cos(i * angle), 0.0f, radius * sin(i * angle));
			Renderer2D::AddLine(lastVertex, vertex, color);
			lastVertex = vertex;
		}
	}

	void Renderer2D::AddBillboard(const glm::vec3& position, float size, const glm::vec3& color, const glm::mat4& viewMatrix, int entityIndex)
	{
		constexpr glm::mat4x2 genericVertexOffsets = {
			{ -1.0f, -1.0f },
			{ 1.0f, -1.0f },
			{ 1.0f, 1.0f },
			{ -1.0f, 1.0f }
		};

		const glm::vec3& right = { viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0] };
		const glm::vec3& up = { viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1] };

		for (uint8_t i = 0; i < 4; i++)
		{
			auto& vertex = s_Renderer2DData.QuadVertices.emplace_back();

			vertex.Position = glm::vec3(position
				+ size * genericVertexOffsets[i].x * right
				- size * genericVertexOffsets[i].y * up);

			// TODO: Texture Coordinates

			vertex.Color = color;
			vertex.EntityIndex = entityIndex;
		}
	}

	void Renderer2D::AddGrid(int gridSize)
	{
		constexpr float squareSize = 1.0f;
		const float length = 2 * gridSize * squareSize;

		const float start = -gridSize * squareSize;

		float current = start;

		constexpr glm::vec3 lineColor(0.3f);
		constexpr glm::vec3 boldLineColor(0.7f);
		constexpr glm::vec3 redLineColor(1.0f, 0.235f, 0.286f);
		constexpr glm::vec3 blueLineColor(0.286f, 0.235f, 1.0f);

		for (int i = -gridSize; i <= gridSize; i++)
		{
			bool is_bold = i == 0;

			Renderer2D::AddLine({ current, 0.0f, start }, { current, 0.0f, start + length }, is_bold ? blueLineColor : lineColor);
			Renderer2D::AddLine({ start, 0.0f, current }, { start + length, 0.0f, current }, is_bold ? redLineColor : lineColor);
			current += squareSize;
		}
	}

	void Renderer2D::AddAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color)
	{
		const glm::vec3 vertices[] = {
			transform * glm::vec4(aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f),
			transform * glm::vec4(aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f),
			transform * glm::vec4(aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f),
			transform * glm::vec4(aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f),
			transform * glm::vec4(aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f),
			transform * glm::vec4(aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f),
			transform * glm::vec4(aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f),
			transform * glm::vec4(aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f),
		};

		// Define the edges as pairs of vertex indices
		const uint32_t edges[][2] = {
			{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
			{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
			{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
		};

		for (uint8_t i = 0; i < 12; i++)
			Renderer2D::AddLine(vertices[edges[i][0]], vertices[edges[i][1]], color);
	}

	void Renderer2D::AddText(const std::string& text, const Ref<Font>& fontAsset, const glm::mat4& transform, const TextParams& textParams, int entityIndex)
	{
		const Ref<Font> font = fontAsset ? fontAsset : Font::GetDefault();

		const auto it = s_Renderer2DData.FontHandleToBatchIndex.find(font->Handle);
		uint32_t batchIndex = -1;

		if (it == s_Renderer2DData.FontHandleToBatchIndex.end())
		{
			// Begin Batch
			auto& batch = s_Renderer2DData.TextBatches.emplace_back();
			batch.FontAtlasTexture = font->GetAtlasTexture();

			batchIndex = s_Renderer2DData.TextBatches.size() - 1;
			s_Renderer2DData.FontHandleToBatchIndex[font->Handle] = batchIndex;
		}
		else
		{
			batchIndex = it->second;
		}

		auto& batch = s_Renderer2DData.TextBatches[batchIndex];
		batch.TextVertices.reserve(batch.TextVertices.size() + text.size() * 4);

		// Adding the text
		const auto& fontGeometry = font->GetMSDFFontData()->FontGeometry;
		const auto& metrics = fontGeometry.getMetrics();

		Ref<Texture2D> fontAtlas = font->GetAtlasTexture();

		double x = 0.0, y = 0.0;
		const double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);

		const double spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();

		for (uint32_t i = 0; i < text.size(); i++)
		{
			const char character = text[i];

			if (character == '\r')
				continue;

			if (character == '\n')
			{
				x = 0.0;
				y -= fsScale * fontGeometry.getMetrics().lineHeight + textParams.LineSpacing;
				continue;
			}

			if (character == ' ')
			{
				double advance = spaceGlyphAdvance;
				if (i < text.size() - 1)
				{
					const char nextCharacter = text[i + 1];
					fontGeometry.getAdvance(advance, character, nextCharacter);
				}

				x += fsScale * advance + textParams.Kerning;
				continue;
			}

			if (character == '\t')
			{
				constexpr double tabsize = 4.0;
				x += tabsize * (fsScale * spaceGlyphAdvance + textParams.Kerning);
				continue;
			}

			auto glyph = fontGeometry.getGlyph(character);

			if (!glyph)
			{
				FBY_WARN("Unable to load glyph: {}", character);
				glyph = fontGeometry.getGlyph('?');

				if (!glyph)
				{
					FBY_ERROR("Unable to load glyph: ?");
					return;
				}
			}

			double al, ab, ar, at;
			glyph->getQuadAtlasBounds(al, ab, ar, at);

			double pl, pb, pr, pt;
			glyph->getQuadPlaneBounds(pl, pb, pr, pt);

			pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
			pl += x, pb += y, pr += x, pt += y;

			const float texelWidth = 1.0f / (float)fontAtlas->GetImageSpecification().Width;
			const float texelHeight = 1.0f / (float)fontAtlas->GetImageSpecification().Height;

			al *= texelWidth, ab *= texelHeight, ar *= texelWidth, at *= texelHeight;

			const glm::vec2 textCharacterVertexPositions[] = {
				{ pl, pb },
				{ pl, pt },
				{ pr, pt },
				{ pr, pb },
			};

			const glm::vec2 textCharacterTextureCoordinates[] = {
				{ al, ab },
				{ al, at },
				{ ar, at },
				{ ar, ab },
			};

			// Add single character
			for (uint8_t i = 0; i < 4; i++)
			{
				auto& vertex = batch.TextVertices.emplace_back();

				vertex.Position = transform * glm::vec4(textCharacterVertexPositions[i], 0.0f, 1.0f);

				vertex.Color = textParams.Color;
				vertex.TextureCoordinates = textCharacterTextureCoordinates[i];
				vertex.EntityIndex = entityIndex;
			}

			if (i < text.size() - 1)
			{
				double advance;
				const char nextCharacter = text[i + 1];
				fontGeometry.getAdvance(advance, character, nextCharacter);

				x += fsScale * advance + textParams.Kerning;
			}
		}
	}

	void Renderer2D::BeginScene(VkDescriptorSet globalDescriptorSet)
	{
		s_GlobalDescriptorSet = globalDescriptorSet;
		s_Renderer2DData.QuadVertexBufferOffset = 0;
		s_Renderer2DData.TextVertexBufferOffset = 0;
	}

	// Make sure to call this when you have accumulated all the quads having the same texture map or none
	void Renderer2D::FlushQuads()
	{
		if (s_Renderer2DData.QuadVertices.size())
		{
			FBY_ASSERT(s_Renderer2DData.QuadVertices.size() <= MAX_QUAD_VERTICES, "MAX_QUAD_VERTICES limit reached!");
			s_Renderer2DData.QuadVertexBuffer->WriteToBuffer(s_Renderer2DData.QuadVertices.data(), s_Renderer2DData.QuadVertices.size() * sizeof(QuadVertex), s_Renderer2DData.QuadVertexBufferOffset);

			auto vertexBuffer = s_Renderer2DData.QuadVertexBuffer->GetVulkanBuffer();
			auto indexBuffer = s_Renderer2DData.QuadIndexBuffer->GetVulkanBuffer();
			auto vulkanPipeline = s_Renderer2DData.QuadPipeline->GetVulkanPipeline();
			auto pipelineLayout = s_Renderer2DData.QuadPipeline->GetVulkanPipelineLayout();
			auto indexCount = 6 * (uint32_t)s_Renderer2DData.QuadVertices.size() / 4;

			Renderer::Submit([vulkanPipeline, pipelineLayout, globalDescriptorSet = s_GlobalDescriptorSet, descSet = s_Renderer2DData.TextureMap->CreateOrGetDescriptorSet(), vertexBuffer, offset = s_Renderer2DData.QuadVertexBufferOffset, indexBuffer, indexCount](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
				Renderer::RT_BindPipeline(cmdBuffer, vulkanPipeline);
				VkDescriptorSet descriptorSets[] = { globalDescriptorSet, descSet };
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2, descriptorSets, 0, nullptr);

				VkDeviceSize offsets[] = { offset };
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
			});
			s_Renderer2DData.QuadVertexBufferOffset += s_Renderer2DData.QuadVertices.size() * sizeof(QuadVertex);
			s_Renderer2DData.QuadVertices.clear();
		}
	}

	void Renderer2D::EndScene()
	{
		if (s_Renderer2DData.LineVertices.size())
		{
			FBY_ASSERT(s_Renderer2DData.LineVertices.size() <= 2 * MAX_LINES, "MAX_LINES limit reached!");
			s_Renderer2DData.LineVertexBuffer->WriteToBuffer(s_Renderer2DData.LineVertices.data(), s_Renderer2DData.LineVertices.size() * sizeof(LineVertex), 0);

			uint32_t vertexCount = (uint32_t)s_Renderer2DData.LineVertices.size();
			const VkBuffer vertexBuffer = s_Renderer2DData.LineVertexBuffer->GetVulkanBuffer();
			const VkPipelineLayout pipelineLayout = s_Renderer2DData.LinePipeline->GetVulkanPipelineLayout();
			const VkPipeline vulkanPipeline = s_Renderer2DData.LinePipeline->GetVulkanPipeline();

			Renderer::Submit([vulkanPipeline, pipelineLayout, globalDescriptorSet = s_GlobalDescriptorSet, vertexBuffer, vertexCount](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
				Renderer::RT_BindPipeline(cmdBuffer, vulkanPipeline);
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalDescriptorSet, 0, nullptr);

				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);

				vkCmdDraw(cmdBuffer, vertexCount, 1, 0, 0);
			});
			s_Renderer2DData.LineVertices.clear();
		}

		// Text Rendering
		for (const auto& batch : s_Renderer2DData.TextBatches)
		{
			if (batch.TextVertices.size())
			{
				FBY_ASSERT(batch.TextVertices.size() <= MAX_QUAD_INDICES, "MAX_QUAD_INDICES reached!");
				s_Renderer2DData.TextVertexBuffer->WriteToBuffer(batch.TextVertices.data(), batch.TextVertices.size() * sizeof(TextVertex), s_Renderer2DData.TextVertexBufferOffset);

				const VkBuffer vertexBuffer = s_Renderer2DData.TextVertexBuffer->GetVulkanBuffer();
				const VkBuffer indexBuffer = s_Renderer2DData.TextIndexBuffer->GetVulkanBuffer();
				const VkPipelineLayout pipelineLayout = s_Renderer2DData.TextPipeline->GetVulkanPipelineLayout();
				const VkPipeline vulkanPipeline = s_Renderer2DData.TextPipeline->GetVulkanPipeline();
				const uint32_t vertexCount = (uint32_t)batch.TextVertices.size();
				const uint32_t indexCount = 6 * (uint32_t)batch.TextVertices.size() / 4;

				Renderer::Submit([vulkanPipeline, pipelineLayout, globalDescriptorSet = s_GlobalDescriptorSet, descSet = batch.FontAtlasTexture->CreateOrGetDescriptorSet(), vertexBuffer, offset = s_Renderer2DData.TextVertexBufferOffset, indexBuffer, indexCount](VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
					Renderer::RT_BindPipeline(cmdBuffer, vulkanPipeline);
					VkDescriptorSet descriptorSets[] = { globalDescriptorSet, descSet };
					vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2, descriptorSets, 0, nullptr);

					VkDeviceSize offsets[] = { offset };
					vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
					vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
					vkCmdDrawIndexed(cmdBuffer, indexCount, 1, 0, 0, 0);
				});
				s_Renderer2DData.TextVertexBufferOffset += batch.TextVertices.size() * sizeof(TextVertex);
			}
		}
		s_Renderer2DData.TextBatches.clear();
		s_Renderer2DData.FontHandleToBatchIndex.clear();

		s_GlobalDescriptorSet = VK_NULL_HANDLE;
	}

	void Renderer2D::Shutdown()
	{
		s_Renderer2DData.LinePipeline = nullptr;
		s_Renderer2DData.LineVertexBuffer = nullptr;

		s_Renderer2DData.QuadPipeline = nullptr;
		s_Renderer2DData.QuadVertexBuffer = nullptr;
		s_Renderer2DData.QuadIndexBuffer = nullptr;

		s_Renderer2DData.TextPipeline = nullptr;
		s_Renderer2DData.TextVertexBuffer = nullptr;
		s_Renderer2DData.TextIndexBuffer = nullptr;
		s_Renderer2DData.TextureMap = nullptr;
	}

} // namespace Flameberry
