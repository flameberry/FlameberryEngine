#pragma once

#include <vector>
#include "OpenGL/OpenGLVertex.h"

namespace Flameberry {
    struct Mesh
    {
        std::vector<OpenGLVertex> Vertices;
        std::vector<uint32_t> Indices;

        Mesh() = default;
        Mesh(const std::vector<OpenGLVertex>& vertices, const std::vector<uint32_t>& indices) : Vertices(vertices), Indices(indices) {}
    };
}