#include "OpenGLTexture.h"

#include <glad/glad.h>
#include "OpenGLRenderCommand.h"

namespace Flameberry {
    OpenGLTexture::OpenGLTexture(const std::string& filePath)
        : m_TextureID(0)
    {
        m_TextureID = OpenGLRenderCommand::CreateTexture(filePath);
    }

    OpenGLTexture::~OpenGLTexture()
    {
        // glDeleteTextures(1, &m_TextureID); // TODO: Don't know why this causes problems
    }

    void OpenGLTexture::BindTextureUnit(uint32_t unit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
    }

    void OpenGLTexture::Unbind()
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}