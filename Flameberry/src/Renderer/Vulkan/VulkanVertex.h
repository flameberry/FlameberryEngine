#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>

namespace Flameberry {
    enum class VertexInputAttribute { NONE = 0, FLOAT, VEC2F, VEC3F, VEC4F, INT, VEC2I, VEC3I };

    class VertexInputAttributeLayout
    {
    public:
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

        static std::vector<VkVertexInputAttributeDescription> CreateVertexInputAttributeDescriptions(const std::vector<VertexInputAttribute>& attributes)
        {
            std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions(attributes.size());
            uint32_t offset = 0;
            for (uint32_t i = 0; i < attributes.size(); i++)
            {
                vertexAttributeDescriptions[i].binding = 0;
                vertexAttributeDescriptions[i].location = i;
                vertexAttributeDescriptions[i].format = GetVertexAttributeFormat(attributes[i]);
                vertexAttributeDescriptions[i].offset = offset;

                offset += GetVertexInputAttributeSize(attributes[i]);
            }
            return vertexAttributeDescriptions;
        }
    };

    struct VulkanVertex
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec3 normal;
        glm::vec2 textureUV;

        VulkanVertex()
            : position(0.0f), color(1.0f), normal(0.0f), textureUV(0.0f)
        {
        }

        VulkanVertex(const glm::vec3& position, const glm::vec4& color, const glm::vec3& normal, const glm::vec2& textureUV)
            : position(position), color(color), normal(normal), textureUV(textureUV)
        {
        }

        bool operator==(const VulkanVertex& vertex) const
        {
            return this->position == vertex.position && this->color == vertex.color && this->normal == vertex.normal && this->textureUV == vertex.textureUV;
        }

        bool operator!=(const VulkanVertex& vertex) const
        {
            return !(*this == vertex);
        }

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription vk_vertex_input_binding_description{};
            vk_vertex_input_binding_description.binding = 0;
            vk_vertex_input_binding_description.stride = sizeof(VulkanVertex);
            vk_vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return vk_vertex_input_binding_description;
        }

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
        {
            return VertexInputAttributeLayout::CreateVertexInputAttributeDescriptions({
                Flameberry::VertexInputAttribute::VEC3F,
                Flameberry::VertexInputAttribute::VEC4F,
                Flameberry::VertexInputAttribute::VEC3F,
                Flameberry::VertexInputAttribute::VEC2F
                }
            );
        }
    };
}
