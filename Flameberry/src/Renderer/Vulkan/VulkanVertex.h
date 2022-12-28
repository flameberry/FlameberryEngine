#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>

namespace Flameberry {
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

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription vk_vertex_input_binding_description{};
            vk_vertex_input_binding_description.binding = 0;
            vk_vertex_input_binding_description.stride = sizeof(VulkanVertex);
            vk_vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return vk_vertex_input_binding_description;
        }

        static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 4> vk_vertex_attribute_descriptions{};

            vk_vertex_attribute_descriptions[0].binding = 0;
            vk_vertex_attribute_descriptions[0].location = 0;
            vk_vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            vk_vertex_attribute_descriptions[0].offset = offsetof(VulkanVertex, position);

            vk_vertex_attribute_descriptions[1].binding = 0;
            vk_vertex_attribute_descriptions[1].location = 1;
            vk_vertex_attribute_descriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            vk_vertex_attribute_descriptions[1].offset = offsetof(VulkanVertex, color);

            vk_vertex_attribute_descriptions[2].binding = 0;
            vk_vertex_attribute_descriptions[2].location = 2;
            vk_vertex_attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            vk_vertex_attribute_descriptions[2].offset = offsetof(VulkanVertex, normal);

            vk_vertex_attribute_descriptions[3].binding = 0;
            vk_vertex_attribute_descriptions[3].location = 3;
            vk_vertex_attribute_descriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
            vk_vertex_attribute_descriptions[3].offset = offsetof(VulkanVertex, textureUV);

            return vk_vertex_attribute_descriptions;
        }
    };
}