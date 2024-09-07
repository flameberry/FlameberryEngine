#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace Flameberry {

	struct MeshVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TextureUV;
		glm::vec3 Tangent, BiTangent;

		MeshVertex()
			: Position(0.0f), Normal(0.0f), TextureUV(0.0f), Tangent(0.0f), BiTangent(0.0f)
		{
		}

		MeshVertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& textureUV)
			: Position(position), Normal(normal), TextureUV(textureUV), Tangent(0.0f), BiTangent(0.0f)
		{
		}

		MeshVertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& textureUV, const glm::vec3& tangent, const glm::vec3& bitangent)
			: Position(position), Normal(normal), TextureUV(textureUV), Tangent(tangent), BiTangent(bitangent)
		{
		}

		bool operator==(const MeshVertex& vertex) const
		{
			return this->Position == vertex.Position && this->Normal == vertex.Normal && this->TextureUV == vertex.TextureUV;
		}

		bool operator!=(const MeshVertex& vertex) const
		{
			return !(*this == vertex);
		}

		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription vk_vertex_input_binding_description{};
			vk_vertex_input_binding_description.binding = 0;
			vk_vertex_input_binding_description.stride = sizeof(MeshVertex);
			vk_vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return vk_vertex_input_binding_description;
		}
	};

} // namespace Flameberry
