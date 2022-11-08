#pragma once

#include <vector>
#include "OpenGL/OpenGLVertex.h"

namespace Flameberry {
    class Mesh
    {
    public:
        Mesh();
        Mesh(const std::vector<OpenGLVertex>& vertices, const std::vector<uint32_t>& indices);
        ~Mesh();
        void Draw(const glm::mat4& transform);
        void Invalidate();
    public:
        std::vector<OpenGLVertex> Vertices;
        std::vector<uint32_t> Indices;
        std::vector<uint32_t> TextureIDs;
    private:
        uint32_t m_VertexArrayID, m_VertexBufferID, m_IndexBufferID, m_ShaderProgramID;
    };
}