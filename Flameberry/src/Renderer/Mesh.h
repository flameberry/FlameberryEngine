#pragma once

#include <vector>

#include "OpenGL/OpenGLVertex.h"
#include "OpenGL/OpenGLShader.h"
#include "OpenGL/OpenGLTexture.h"

#include "PerspectiveCamera.h"
#include "Light.h"
#include "Material.h"

#include "ECS/Component.h"

namespace Flameberry {
    class Mesh
    {
    public:
        Mesh();
        Mesh(const std::vector<OpenGLVertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name = "default_mesh_name");
        ~Mesh();
        void Draw(const std::shared_ptr<OpenGLShader>& shader, const TransformComponent& transform, const Material& material, int entityID = -1);
        void Invalidate();
    public:
        std::vector<OpenGLVertex> Vertices;
        std::vector<uint32_t> Indices;
        std::string Name;
    private:
        uint32_t m_VertexArrayID, m_VertexBufferID, m_IndexBufferID, m_ShaderProgramID;
        int m_EntityID = -1;
        float m_TextureIndex = -1.0f;
    };
}