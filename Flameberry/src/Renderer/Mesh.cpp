#include "Mesh.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "OpenGL/OpenGLRenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Flameberry {
    Mesh::Mesh()
        : m_VertexArrayID(0), m_VertexBufferID(0), m_IndexBufferID(0), m_ShaderProgramID(0)
    {
    }

    Mesh::Mesh(const std::vector<OpenGLVertex>& vertices, const std::vector<uint32_t>& indices)
        : Vertices(vertices), Indices(indices), m_VertexArrayID(0), m_VertexBufferID(0), m_IndexBufferID(0), m_ShaderProgramID(0)
    {
        Invalidate();
    }

    void Mesh::Invalidate()
    {
        if (Vertices.size() && Indices.size() && m_VertexArrayID && m_VertexBufferID && m_IndexBufferID && m_ShaderProgramID)
            return;

        glGenVertexArrays(1, &m_VertexArrayID);
        glBindVertexArray(m_VertexArrayID);

        glGenBuffers(1, &m_VertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, 1000000 * sizeof(OpenGLVertex), nullptr, GL_DYNAMIC_DRAW);

        glBindVertexArray(m_VertexArrayID);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, normal));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, texture_uv));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, texture_index));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 1, GL_INT, GL_FALSE, 1 * sizeof(int) + 13 * sizeof(float), (void*)offsetof(OpenGLVertex, entityID));

        glGenBuffers(1, &m_IndexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(uint32_t), Indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(m_VertexArrayID);

        m_ShaderProgramID = OpenGLRenderCommand::CreateShader(FL_PROJECT_DIR"Flameberry/assets/shaders/Default.glsl");
        glUseProgram(m_ShaderProgramID);
        int samplers[16];
        for (uint32_t i = 0; i < 16; i++)
            samplers[i] = i;
        glUniform1iv(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_TextureSamplers"), 16, samplers);
        glUseProgram(0);
    }

    void Mesh::Draw(const glm::mat4& transform)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Vertices.size() * sizeof(OpenGLVertex), Vertices.data());

        for (uint16_t i = 0; i < TextureIDs.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, TextureIDs[i]);
        }

        glUseProgram(m_ShaderProgramID);
        glUniformMatrix4fv(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_ModelMatrix"), 1, GL_FALSE, glm::value_ptr(transform));

        glBindVertexArray(m_VertexArrayID);
        glDrawElements(GL_TRIANGLES, (int)Indices.size(), GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    Mesh::~Mesh()
    {
    }
}