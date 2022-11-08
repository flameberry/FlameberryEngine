#pragma once

#include <glm/glm.hpp>

namespace Flameberry {
    struct OpenGLVertex2D
    {
        glm::vec3 position;   // Position from -1.0f to 1.0f on both x-axis and y-axis
        glm::vec4 color;      // Color in rgba format, each channel ranging from 0.0f to 1.0f
        glm::vec2 texture_uv; // Texture coordinates ranging from 0.0f to 1.0f
        float texture_index;  // Texture index which will be used as opengl texture slot to which the texture will be bound
        int entityID;

        OpenGLVertex2D()
            : position(0.0f), color(1.0f), texture_uv(0.0f), texture_index(-1.0f), entityID(-1)
        {}
    };

    struct OpenGLVertex
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec3 normal;
        glm::vec2 texture_uv;
        float texture_index;
        int entityID;

        /// Default Constructor
        OpenGLVertex()
            : position(0.0f), color(1.0f), normal(0.0f), texture_uv(0.0f), texture_index(-1.0f), entityID(-1)
        {}
    };
}