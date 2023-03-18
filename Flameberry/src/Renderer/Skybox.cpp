#include "Skybox.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "OpenGL/OpenGLRenderCommand.h"
#include "OpenGL/OpenGLRenderer3D.h"
#include "OpenGL/OpenGLUniformBufferIndices.h"

namespace Flameberry {
    Skybox::Skybox()
        : m_VertexArrayID(0), m_VertexBufferID(0), m_IndexBufferID(0), m_ShaderProgramID(0)
    {}

    Skybox::Skybox(const char* folderPath)
        : m_FolderPath(folderPath), m_VertexArrayID(0), m_VertexBufferID(0), m_IndexBufferID(0), m_ShaderProgramID(0)
    {
        OpenGLShaderBinding cameraBinding{};
        cameraBinding.blockBindingIndex = FL_UNIFORM_BLOCK_BINDING_CAMERA;
        cameraBinding.blockName = "Camera";

        m_SkyboxShader = OpenGLShader::Create(FL_PROJECT_DIR"Flameberry/assets/shaders/opengl/skybox.glsl", { cameraBinding });
        m_SkyboxShader->Bind();
        m_SkyboxShader->PushUniformInt("u_SamplerCube", 0);
        m_SkyboxShader->Unbind();
        Load(folderPath);
    }

    void Skybox::Load(const char* folderPath)
    {
        if (m_VertexArrayID)
        {
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

        m_SkyboxTexture = OpenGLTexture::Create(folderPath);
    }

    void Skybox::BindCubeMapTextureToUnit(uint32_t unit)
    {
        m_SkyboxTexture->BindTextureUnit(unit);
    }

    void Skybox::OnDraw(const PerspectiveCamera& camera)
    {
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);

        m_SkyboxTexture->BindTextureUnit(0);

        glm::mat4 viewProjectionMatrix = camera.GetProjectionMatrix() * glm::mat4(glm::mat3(camera.GetViewMatrix()));

        m_SkyboxShader->Bind();
        m_SkyboxShader->PushUniformMatrix4f("u_ViewProjectionMatrix", 1, GL_FALSE, glm::value_ptr(viewProjectionMatrix));

        glBindVertexArray(m_VertexArrayID);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
    }

    Skybox::~Skybox()
    {
    }
}
