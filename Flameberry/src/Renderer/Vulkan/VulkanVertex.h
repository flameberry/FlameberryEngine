#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>

namespace Flameberry {
    struct VulkanVertex
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 textureUV;

        VulkanVertex()
            : position(0.0f), color(1.0f)
        {
        }

        VulkanVertex(const glm::vec3& position, glm::vec4& color)
            : position(position), color(color)
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

        static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> vk_vertex_attribute_descriptions{};

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
            vk_vertex_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            vk_vertex_attribute_descriptions[2].offset = offsetof(VulkanVertex, textureUV);

            return vk_vertex_attribute_descriptions;
        }
    };
}