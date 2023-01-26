#include "OpenGLFramebuffer.h"

#include "Core/Core.h"

namespace Flameberry {
    OpenGLFramebuffer::OpenGLFramebuffer(const OpenGLFramebufferSpecification& specification)
        : m_FramebufferSpec(specification), m_FramebufferID(0)
    {
        Invalidate();
    }

    void OpenGLFramebuffer::Invalidate()
    {
        if (m_FramebufferID)
        {
            glDeleteFramebuffers(1, &m_FramebufferID);
            glDeleteTextures((uint32_t)m_FramebufferAttachmentIDs.size(), m_FramebufferAttachmentIDs.data());
        }

        glGenFramebuffers(1, &m_FramebufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);

        std::vector<uint32_t> attachmentNames;

        m_FramebufferAttachmentIDs.resize(m_FramebufferSpec.Attachments.size());
        for (uint32_t i = 0; i < m_FramebufferSpec.Attachments.size(); i++)
        {
            auto& attachment = m_FramebufferSpec.Attachments[i];
            auto& ID = m_FramebufferAttachmentIDs[i];

            glGenTextures(1, &ID);
            glBindTexture(attachment.Target, ID);
            glTexImage2D(attachment.Target, 0, attachment.InternalFormat, m_FramebufferSpec.FramebufferSize.x, m_FramebufferSpec.FramebufferSize.y, 0, attachment.Format, attachment.Type, nullptr);
            attachment.SetupTextureProperties();
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment.Attachment, attachment.Target, ID, 0);

            if (attachment.IsColorAttachment)
                attachmentNames.emplace_back(attachment.Attachment);
        }

        glDrawBuffers((uint32_t)attachmentNames.size(), attachmentNames.data());

        uint32_t status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        FL_ASSERT(status == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::SetFramebufferSize(float width, float height)
    {
        m_FramebufferSpec.FramebufferSize = { width, height };
    }

    void OpenGLFramebuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);
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
        glDeleteTextures((uint32_t)m_FramebufferAttachmentIDs.size(), m_FramebufferAttachmentIDs.data());
        glDeleteFramebuffers(1, &m_FramebufferID);
    }
}
