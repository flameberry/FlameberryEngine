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
        Mesh(const char* filePath);
        Mesh(const std::vector<OpenGLVertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name = "default_mesh_name");
        ~Mesh();
        void Draw(const std::shared_ptr<OpenGLShader>& shader, const TransformComponent& transform, const Material& material);
        void DrawForShadowPass(const std::shared_ptr<OpenGLShader>& shader, const TransformComponent& transform);
        void DrawForMousePicking(const std::shared_ptr<OpenGLShader>& shader, const TransformComponent& transform, int entityID = -1);
        void Invalidate();
    public:
        std::vector<OpenGLVertex> Vertices;
        std::vector<uint32_t> Indices;
        std::string Name;
    private:
        std::string m_FilePath;
        uint32_t m_VertexArrayID, m_VertexBufferID, m_IndexBufferID;
        int m_EntityID = -1;
    };
}