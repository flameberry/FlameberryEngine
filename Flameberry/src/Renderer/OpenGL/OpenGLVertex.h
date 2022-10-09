#pragma once

#include <glm/glm.hpp>

struct OpenGLVertex
{
    glm::vec3 position;   // Position from -1.0f to 1.0f on both x-axis and y-axis
    glm::vec4 color;      // Color in rgba format, each channel ranging from 0.0f to 1.0f
    glm::vec2 texture_uv; // Texture coordinates ranging from 0.0f to 1.0f
    float texture_index;  // Texture index which will be used as opengl texture slot to which the texture will be bound
    int entityID;

    OpenGLVertex()
        : position(0.0f), color(1.0f), texture_uv(0.0f), texture_index(-1.0f), entityID(-1)
    {}
};