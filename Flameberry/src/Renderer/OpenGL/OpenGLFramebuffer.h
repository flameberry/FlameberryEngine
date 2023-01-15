#pragma once

#include <memory>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace Flameberry {
    struct OpenGLFramebufferAttachment {
        uint32_t InternalFormat, Format, Type, Attachment;
        bool IsColorAttachment = true;
        void(*SetupTextureProperties)();
    };

    struct OpenGLFramebufferSpecification
    {
        glm::vec2 FramebufferSize;
        std::vector<OpenGLFramebufferAttachment> Attachments;
    };

    // Class which deals with OpenGL Framebuffer
    class OpenGLFramebuffer
    {
    public:
        template<typename... Args>
        static std::shared_ptr<OpenGLFramebuffer> Create(Args... args) { return std::make_shared<OpenGLFramebuffer>(std::forward<Args>(args)...); }

        OpenGLFramebuffer(const OpenGLFramebufferSpecification& specification);
        ~OpenGLFramebuffer();

        // Recreates the Framebuffer object using the `m_FramebufferSize` variable
        void     Invalidate();
        // Sets the Framebuffer Size, but to take effect the Framebuffer object must be recreated using the `OnUpdate()` function
        void     SetFramebufferSize(float width, float height);
        const glm::vec2& GetFramebufferSize() { return m_FramebufferSpec.FramebufferSize; };
        // Returns the opengl texture Id of texture made using the Framebuffer object
        uint32_t GetColorAttachmentID() const { return m_FramebufferAttachmentIDs[0]; };
        // Binds the Framebuffer object
        void     Bind() const;
        // Unbinds the Framebuffer object
        void     Unbind() const;

        void ClearEntityIDAttachment();
        int ReadPixel(GLenum index, int x, int y);
    private:
        OpenGLFramebufferSpecification m_FramebufferSpec;
        std::vector<uint32_t> m_FramebufferAttachmentIDs;
        uint32_t m_FramebufferID;
    };
}
