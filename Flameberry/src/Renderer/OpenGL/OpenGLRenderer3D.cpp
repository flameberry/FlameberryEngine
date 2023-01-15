#include "OpenGLRenderer3D.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "OpenGLRenderCommand.h"
#include "Core/Core.h"

namespace Flameberry {
    OpenGLRenderer3D::OpenGLRenderer3D()
        // : m_CameraUniformBuffer(sizeof(CameraUniformBufferData), nullptr, GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW)
    {
        // m_CameraUniformBuffer.BindBufferBase(FL_UNIFORM_BLOCK_BINDING_CAMERA);
    }

    void OpenGLRenderer3D::Begin(const PerspectiveCamera& camera)
    {
        // m_UniformBufferData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();

        /* Set Projection Matrix in GPU memory, for all shader programs to access it */
        // m_CameraUniformBuffer.Bind();
        // m_CameraUniformBuffer.BufferSubData(&m_UniformBufferData, sizeof(CameraUniformBufferData), 0);
    }

    void OpenGLRenderer3D::End()
    {
        // m_CameraUniformBuffer.Unbind();
    }

    void OpenGLRenderer3D::Init()
    {
    }

    void OpenGLRenderer3D::CleanUp()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
}
