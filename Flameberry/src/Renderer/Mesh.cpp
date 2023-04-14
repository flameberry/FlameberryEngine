#include "Mesh.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "OpenGL/OpenGLRenderCommand.h"
#include "OpenGL/OpenGLRenderer3D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Core/Core.h"

namespace Flameberry {
    Mesh::Mesh(const std::string& filePath)
        : m_FilePath(filePath), m_VertexArrayID(0), m_VertexBufferID(0), m_IndexBufferID(0)
    {
        const auto& [v, i] = OpenGLRenderCommand::LoadModel(filePath);
        Vertices = std::move(v);
        Indices = std::move(i);
        Invalidate();

        uint32_t lengthSlash = m_FilePath.find_last_of('/') + 1;
        uint32_t lengthDot = m_FilePath.find_last_of('.');
        Name = m_FilePath.substr(lengthSlash, lengthDot - lengthSlash);

        FL_TRACE("Allocated {0}, {1} bytes for {2}: Vertices, Indices", Vertices.size() * sizeof(OpenGLVertex), Indices.size() * sizeof(uint32_t), Name);
    }

    Mesh::Mesh(const std::vector<OpenGLVertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name)
        : Vertices(vertices), Indices(indices), Name(name), m_VertexArrayID(0), m_VertexBufferID(0), m_IndexBufferID(0)
    {
        Invalidate();
    }

    void Mesh::Invalidate()
    {
        if (m_VertexArrayID && m_VertexBufferID && m_IndexBufferID)
        {
            glDeleteVertexArrays(1, &m_VertexArrayID);
            glDeleteBuffers(1, &m_VertexBufferID);
            glDeleteBuffers(1, &m_IndexBufferID);
            return;
        }

        glGenVertexArrays(1, &m_VertexArrayID);
        glBindVertexArray(m_VertexArrayID);

        glGenBuffers(1, &m_VertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
        // glBufferData(GL_ARRAY_BUFFER, 1000000 * sizeof(OpenGLVertex), nullptr, GL_DYNAMIC_DRAW);
        glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(OpenGLVertex), Vertices.data(), GL_DYNAMIC_DRAW);

        glBindVertexArray(m_VertexArrayID);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)offsetof(OpenGLVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)offsetof(OpenGLVertex, color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)offsetof(OpenGLVertex, normal));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)offsetof(OpenGLVertex, texture_uv));

        glGenBuffers(1, &m_IndexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(uint32_t), Indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(m_VertexArrayID);
        glUseProgram(0);
    }

    void Mesh::Draw(
        const std::shared_ptr<OpenGLShader>& shader,
        const TransformComponent& transform,
        const Material& material
    )
    {
        if (!Vertices.size())
            return;

        if (material.TextureMapEnabled && material.TextureMap)
            material.TextureMap->BindTextureUnit(0);

        shader->Bind();
        shader->PushUniformMatrix4f("u_ModelMatrix", 1, false, glm::value_ptr(transform.GetTransform()));
        shader->PushUniformFloat3("u_Material.Albedo", material.Albedo.x, material.Albedo.y, material.Albedo.z);
        shader->PushUniformFloat("u_Material.Roughness", material.Roughness);
        shader->PushUniformInt("u_Material.Metallic", material.Metallic);
        shader->PushUniformInt("u_Material.TextureMapEnabled", material.TextureMapEnabled);

        glBindVertexArray(m_VertexArrayID);
        glDrawElements(GL_TRIANGLES, (int)Indices.size(), GL_UNSIGNED_INT, 0);
    }

    void Mesh::DrawForShadowPass(const std::shared_ptr<OpenGLShader>& shader, const TransformComponent& transform)
    {
        if (!Vertices.size())
            return;

        shader->Bind();
        shader->PushUniformMatrix4f("u_ModelMatrix", 1, false, glm::value_ptr(transform.GetTransform()));

        glBindVertexArray(m_VertexArrayID);
        glDrawElements(GL_TRIANGLES, (int)Indices.size(), GL_UNSIGNED_INT, 0);
    }

    void Mesh::DrawForMousePicking(
        const std::shared_ptr<OpenGLShader>& shader,
        const TransformComponent& transform,
        int entityID
    )
    {
        if (!Vertices.size())
            return;

        m_EntityID = entityID;

        shader->Bind();
        shader->PushUniformMatrix4f("u_ModelMatrix", 1, false, glm::value_ptr(transform.GetTransform()));
        shader->PushUniformInt("u_EntityID", m_EntityID);

        glBindVertexArray(m_VertexArrayID);
        glDrawElements(GL_TRIANGLES, (int)Indices.size(), GL_UNSIGNED_INT, 0);
    }

    Mesh::~Mesh()
    {
        glDeleteVertexArrays(1, &m_VertexArrayID);
        glDeleteBuffers(1, &m_VertexBufferID);
        glDeleteBuffers(1, &m_IndexBufferID);
    }
}
