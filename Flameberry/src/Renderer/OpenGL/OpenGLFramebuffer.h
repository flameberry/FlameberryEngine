#pragma once

#include <memory>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace Flameberry {
    struct OpenGLFramebufferAttachment {
        uint32_t Target, InternalFormat, Format, Type, Attachment;
        bool IsColorAttachment = true;
        void(*SetupTextureProperties)();

        OpenGLFramebufferAttachment(): Target(GL_TEXTURE_2D), InternalFormat(GL_RGBA8), Format(GL_RGBA), Type(GL_UNSIGNED_BYTE), Attachment(GL_COLOR_ATTACHMENT0), IsColorAttachment(true) {}
    };

    struct OpenGLFramebufferSpecification
    {
        glm::vec2 FramebufferSize;
        std::vector<OpenGLFramebufferAttachment> Attachments;
    };

    class OpenGLFramebuffer
    {
    public:
        template<typename... Args>
        static std::shared_ptr<OpenGLFramebuffer> Create(Args... args) { return std::make_shared<OpenGLFramebuffer>(std::forward<Args>(args)...); }

        OpenGLFramebuffer(const OpenGLFramebufferSpecification& specification);
        ~OpenGLFramebuffer();

        // Recreates the Framebuffer object using the `m_FramebufferSize` variable
        void Invalidate();
        void SetFramebufferSize(float width, float height);
        const glm::vec2& GetFramebufferSize() { return m_FramebufferSpec.FramebufferSize; };
        uint32_t GetColorAttachmentID() const { return m_FramebufferAttachmentIDs[0]; };
        uint32_t GetFramebufferID() const { return m_FramebufferID; }
        void Bind() const;
        void Unbind() const;

        int ReadPixel(GLenum index, int x, int y);
    private:
        OpenGLFramebufferSpecification m_FramebufferSpec;
        std::vector<uint32_t> m_FramebufferAttachmentIDs;
        uint32_t m_FramebufferID;
    };
}
