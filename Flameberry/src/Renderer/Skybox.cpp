#include "Skybox.h"

#include <glad/glad.h>
#include "OpenGL/OpenGLRenderCommand.h"
#include "OpenGL/OpenGLRenderer3D.h"
#include <glm/gtc/type_ptr.hpp>

namespace Flameberry {
    Skybox::Skybox()
        : m_VertexBufferID(0), m_IndexBufferID(0), m_ShaderProgramID(0), m_TextureID(0)
    {}

    Skybox::Skybox(const char* folderPath)
        : m_VertexBufferID(0), m_IndexBufferID(0), m_ShaderProgramID(0), m_TextureID(0)
    {
        Load(folderPath);
    }

    void Skybox::Load(const char* folderPath)
    {
        if (m_VertexArrayID)
        {
            glDeleteTextures(1, &m_TextureID);
            glDeleteVertexArrays(1, &m_VertexArrayID);
            glDeleteBuffers(1, &m_VertexBufferID);
            glDeleteBuffers(1, &m_IndexBufferID);
            glDeleteProgram(m_ShaderProgramID);
        }

        float skyboxVertices[] = {
            //   Coordinates
            -1.0f, -1.0f,  1.0f,//        7--------6
             1.0f, -1.0f,  1.0f,//       /|       /|
             1.0f, -1.0f, -1.0f,//      4--------5 |
            -1.0f, -1.0f, -1.0f,//      | |      | |
            -1.0f,  1.0f,  1.0f,//      | 3------|-2
             1.0f,  1.0f,  1.0f,//      |/       |/
             1.0f,  1.0f, -1.0f,//      0--------1
            -1.0f,  1.0f, -1.0f
        };

        unsigned int skyboxIndices[] = {
            // Right
            1, 2, 6,
            6, 5, 1,
            // Left
            0, 4, 7,
            7, 3, 0,
            // Top
            4, 5, 6,
            6, 7, 4,
            // Bottom
            0, 3, 2,
            2, 1, 0,
            // Back
            0, 1, 5,
            5, 4, 0,
            // Front
            3, 7, 6,
            6, 2, 3
        };

        glGenVertexArrays(1, &m_VertexArrayID);
        glBindVertexArray(m_VertexArrayID);

        glGenBuffers(1, &m_VertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

        glBindVertexArray(m_VertexArrayID);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glGenBuffers(1, &m_IndexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, GL_STATIC_DRAW);

        glBindVertexArray(m_VertexArrayID);

        m_ShaderProgramID = OpenGLRenderCommand::CreateShader(FL_PROJECT_DIR"Flameberry/assets/shaders/Skybox.glsl");
        glUseProgram(m_ShaderProgramID);
        glUniform1i(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_SamplerCube"), 0);

        // Assigning binding to uniform buffers
        uint32_t uniformBlockIndexCamera = glGetUniformBlockIndex(m_ShaderProgramID, "Camera");
        glUniformBlockBinding(m_ShaderProgramID, uniformBlockIndexCamera, FL_UNIFORM_BLOCK_BINDING_CAMERA);

        glUseProgram(0);

        m_TextureID = OpenGLRenderCommand::CreateCubeMap(folderPath);
    }

    void Skybox::OnDraw(const PerspectiveCamera& camera)
    {
        glDepthFunc(GL_LEQUAL);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);

        glUseProgram(m_ShaderProgramID);

        glm::mat4 viewProjectionMatrix = camera.GetProjectionMatrix() * glm::mat4(glm::mat3(camera.GetViewMatrix()));
        glUniformMatrix4fv(OpenGLRenderCommand::GetUniformLocation(m_ShaderProgramID, "u_ViewProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(viewProjectionMatrix));

        glBindVertexArray(m_VertexArrayID);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        glDepthFunc(GL_LESS);
    }

    Skybox::~Skybox()
    {
    }
}
