#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>

namespace Flameberry {
    enum class VertexInputAttribute { NONE = 0, FLOAT, VEC2F, VEC3F, VEC4F, INT, VEC2I, VEC3I };

    static VkFormat GetVertexAttributeFormat(VertexInputAttribute attribute)
    {
        switch (attribute)
        {
        case VertexInputAttribute::NONE:  return VK_FORMAT_UNDEFINED;           break;
        case VertexInputAttribute::FLOAT: return VK_FORMAT_R32_SFLOAT;          break;
        case VertexInputAttribute::VEC2F: return VK_FORMAT_R32G32_SFLOAT;       break;
        case VertexInputAttribute::VEC3F: return VK_FORMAT_R32G32B32_SFLOAT;    break;
        case VertexInputAttribute::VEC4F: return VK_FORMAT_R32G32B32A32_SFLOAT; break;
        case VertexInputAttribute::INT:   return VK_FORMAT_R32_SINT;            break;
        case VertexInputAttribute::VEC2I: return VK_FORMAT_R32G32_SINT;         break;
        case VertexInputAttribute::VEC3I: return VK_FORMAT_R32G32B32_SINT;      break;
        }
        return VK_FORMAT_UNDEFINED;
    }

    static uint32_t GetVertexInputAttributeSize(VertexInputAttribute attribute)
    {
        switch (attribute)
        {
        case VertexInputAttribute::NONE:  return 0;                 break;
        case VertexInputAttribute::FLOAT: return 1 * sizeof(float); break;
        case VertexInputAttribute::VEC2F: return 2 * sizeof(float); break;
        case VertexInputAttribute::VEC3F: return 3 * sizeof(float); break;
        case VertexInputAttribute::VEC4F: return 4 * sizeof(float); break;
        case VertexInputAttribute::INT:   return 1 * sizeof(int);   break;
        case VertexInputAttribute::VEC2I: return 2 * sizeof(int);   break;
        case VertexInputAttribute::VEC3I: return 3 * sizeof(int);   break;
        }
    }

    class VertexInputAttributeLayout
    {
    public:
        VertexInputAttributeLayout(const std::initializer_list<VertexInputAttribute>& attributes = {})
            : m_VertexInputAttributes(attributes)
        {}

        std::vector<VkVertexInputAttributeDescription> CreateVertexInputAttributeDescriptions()
        {
            std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions(m_VertexInputAttributes.size());
            uint32_t offset = 0;
            for (uint32_t i = 0; i < m_VertexInputAttributes.size(); i++)
            {
                vertexAttributeDescriptions[i].binding = 0;
                vertexAttributeDescriptions[i].location = i;
                vertexAttributeDescriptions[i].format = GetVertexAttributeFormat(m_VertexInputAttributes[i]);
                vertexAttributeDescriptions[i].offset = offset;

                offset += GetVertexInputAttributeSize(m_VertexInputAttributes[i]);
            }
            return vertexAttributeDescriptions;
        }
    private:
        std::vector<VertexInputAttribute> m_VertexInputAttributes;
    };

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

        MeshVertex(const glm::vec3& position, const glm::vec4& color, const glm::vec3& normal, const glm::vec2& textureUV)
            : Position(position), Normal(normal), TextureUV(textureUV)
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
}
