#pragma once

#include <vector>

#include "OpenGL/OpenGLVertex.h"
#include "Light.h"
#include "PerspectiveCamera.h"

#include "ECS/Component.h"

namespace Flameberry {
    class Mesh
    {
    public:
        Mesh();
        Mesh(const std::vector<OpenGLVertex>& vertices, const std::vector<uint32_t>& indices);
        ~Mesh();
        void Draw(const glm::mat4& transform);
        void Draw(const glm::mat4& transform, const glm::vec3& cameraPosition, const std::vector<PointLight>& lights);
        void Draw(const TransformComponent& transform, const glm::vec3& cameraPosition, const std::vector<PointLight>& lights, int entityID = -1);
        void Invalidate();
    public:
        std::vector<OpenGLVertex> Vertices;
        std::vector<uint32_t> Indices;
        std::vector<uint32_t> TextureIDs;
    private:
        uint32_t m_VertexArrayID, m_VertexBufferID, m_IndexBufferID, m_ShaderProgramID;
        int m_EntityID = -1;
    };
}