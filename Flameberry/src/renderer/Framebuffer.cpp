#include "Framebuffer.h"
#include "../core/Core.h"
#include <glad/glad.h>

namespace Flameberry {
    std::shared_ptr<Framebuffer> Framebuffer::Create(float width, float height)
    {
        return std::make_shared<Framebuffer>(width, height);
    }

    Framebuffer::Framebuffer(float width, float height)
        : m_FramebufferId(0), m_ColorAttachmentId(0), m_DepthAttachmentId(0), m_FramebufferSize(width, height)
    {
        glGenFramebuffers(1, &m_FramebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferId);

        glGenTextures(1, &m_ColorAttachmentId);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachmentId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_FramebufferSize.x, m_FramebufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachmentId, 0);

        glGenTextures(1, &m_DepthAttachmentId);
        glBindTexture(GL_TEXTURE_2D, m_DepthAttachmentId);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_FramebufferSize.x, m_FramebufferSize.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachmentId, 0);

        FL_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::OnUpdate()
    {
        if (m_FramebufferId)
        {
            glDeleteFramebuffers(1, &m_FramebufferId);
            glDeleteTextures(1, &m_ColorAttachmentId);
            glDeleteTextures(1, &m_DepthAttachmentId);
        }

        glGenFramebuffers(1, &m_FramebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferId);

        glGenTextures(1, &m_ColorAttachmentId);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachmentId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_FramebufferSize.x, m_FramebufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachmentId, 0);

        glGenTextures(1, &m_DepthAttachmentId);
        glBindTexture(GL_TEXTURE_2D, m_DepthAttachmentId);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_FramebufferSize.x, m_FramebufferSize.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachmentId, 0);

        FL_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::SetFramebufferSize(float width, float height)
    {
        m_FramebufferSize = { width, height };
    }

    void Framebuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferId);
        glViewport(0, 0, m_FramebufferSize.x, m_FramebufferSize.y);
    }

    void Framebuffer::Unbind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Framebuffer::~Framebuffer()
    {
        glDeleteTextures(1, &m_ColorAttachmentId);
        glDeleteTextures(1, &m_DepthAttachmentId);
        glDeleteFramebuffers(1, &m_FramebufferId);
    }
}