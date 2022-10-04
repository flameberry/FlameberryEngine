#include "OpenGLFramebuffer.h"

#include "Core/Core.h"

namespace Flameberry {
    std::shared_ptr<OpenGLFramebuffer> OpenGLFramebuffer::Create(float width, float height)
    {
        return std::make_shared<OpenGLFramebuffer>(width, height);
    }

    OpenGLFramebuffer::OpenGLFramebuffer(float width, float height)
        : m_FramebufferId(0), m_ColorAttachmentId(0), m_DepthAttachmentId(0), m_PickingTextureId(0), m_FramebufferSize(width, height)
    {
        Invalidate();
    }

    void OpenGLFramebuffer::Invalidate()
    {
        if (m_FramebufferId)
        {
            glDeleteFramebuffers(1, &m_FramebufferId);
            glDeleteTextures(1, &m_ColorAttachmentId);
            glDeleteTextures(1, &m_PickingTextureId);
            glDeleteTextures(1, &m_DepthAttachmentId);
        }

        glGenFramebuffers(1, &m_FramebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferId);

        // Color Attachment
        glGenTextures(1, &m_ColorAttachmentId);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachmentId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_FramebufferSize.x, m_FramebufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachmentId, 0);

        // Picking Texture Attachment
        glGenTextures(1, &m_PickingTextureId);
        glBindTexture(GL_TEXTURE_2D, m_PickingTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, m_FramebufferSize.x, m_FramebufferSize.y, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, m_PickingTextureId, 0);

        // Depth Attachment
        glGenTextures(1, &m_DepthAttachmentId);
        glBindTexture(GL_TEXTURE_2D, m_DepthAttachmentId);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_FramebufferSize.x, m_FramebufferSize.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachmentId, 0);

        uint32_t buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, buffers);

        FL_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::SetFramebufferSize(float width, float height)
    {
        m_FramebufferSize = { width, height };
    }

    void OpenGLFramebuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferId);
        glViewport(0, 0, m_FramebufferSize.x, m_FramebufferSize.y);
    }

    void OpenGLFramebuffer::ClearEntityIDAttachment()
    {
        int clearValue = -1;
        glClearBufferiv(GL_COLOR, 1, &clearValue);
    }

    int OpenGLFramebuffer::ReadPixel(GLenum index, int x, int y)
    {
        glReadBuffer(index);
        int value;
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &value);
        return value;
    }

    void OpenGLFramebuffer::Unbind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        glDeleteTextures(1, &m_ColorAttachmentId);
        glDeleteTextures(1, &m_DepthAttachmentId);
        glDeleteTextures(1, &m_PickingTextureId);
        glDeleteFramebuffers(1, &m_FramebufferId);
    }
}
