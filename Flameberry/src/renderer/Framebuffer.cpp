#include "Framebuffer.h"
#include "../core/Core.h"
#include <glad/glad.h>

namespace Flameberry {
    std::shared_ptr<Framebuffer> Framebuffer::Create(float width, float height)
    {
        return std::make_shared<Framebuffer>(width, height);
    }

    Framebuffer::Framebuffer(float width, float height)
        : M_FramebufferId(0), M_ColorAttachmentId(0), M_DepthAttachmentId(0), M_FramebufferSize(width, height)
    {
        glGenFramebuffers(1, &M_FramebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, M_FramebufferId);

        glGenTextures(1, &M_ColorAttachmentId);
        glBindTexture(GL_TEXTURE_2D, M_ColorAttachmentId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, M_FramebufferSize.x, M_FramebufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, M_ColorAttachmentId, 0);

        glGenTextures(1, &M_DepthAttachmentId);
        glBindTexture(GL_TEXTURE_2D, M_DepthAttachmentId);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, M_FramebufferSize.x, M_FramebufferSize.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, M_DepthAttachmentId, 0);

        FL_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::OnUpdate()
    {
        if (M_FramebufferId)
        {
            glDeleteFramebuffers(1, &M_FramebufferId);
            glDeleteTextures(1, &M_ColorAttachmentId);
            glDeleteTextures(1, &M_DepthAttachmentId);
        }

        glGenFramebuffers(1, &M_FramebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, M_FramebufferId);

        glGenTextures(1, &M_ColorAttachmentId);
        glBindTexture(GL_TEXTURE_2D, M_ColorAttachmentId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, M_FramebufferSize.x, M_FramebufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, M_ColorAttachmentId, 0);

        glGenTextures(1, &M_DepthAttachmentId);
        glBindTexture(GL_TEXTURE_2D, M_DepthAttachmentId);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, M_FramebufferSize.x, M_FramebufferSize.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, M_DepthAttachmentId, 0);

        FL_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::SetFramebufferSize(float width, float height)
    {
        M_FramebufferSize = { width, height };
    }

    void Framebuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, M_FramebufferId);
        glViewport(0, 0, M_FramebufferSize.x, M_FramebufferSize.y);
    }

    void Framebuffer::Unbind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Framebuffer::~Framebuffer()
    {
        glDeleteTextures(1, &M_ColorAttachmentId);
        glDeleteTextures(1, &M_DepthAttachmentId);
        glDeleteFramebuffers(1, &M_FramebufferId);
    }
}