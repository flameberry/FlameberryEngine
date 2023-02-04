#include "OpenGLRenderer3D.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "OpenGLRenderCommand.h"
#include "Core/Core.h"

namespace Flameberry {
    OpenGLRenderer3D::OpenGLRenderer3D()
    {
    }

    void OpenGLRenderer3D::Begin(const PerspectiveCamera& camera)
    {
    }

    void OpenGLRenderer3D::End()
    {
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
