#pragma once
#include <glm/glm.hpp>
#include <memory>

namespace Flameberry {
    /// Class which deals with OpenGL Framebuffer
    class Framebuffer
    {
    public:
        static std::shared_ptr<Framebuffer> Create(float width = 1280.0f, float height = 720.0f);

        Framebuffer(float width = 1280.0f, float height = 720.0f);
        ~Framebuffer();

        /// Recreates the Framebuffer object using the `M_FramebufferSize` variable
        void     OnUpdate();
        /// Sets the Framebuffer Size, but to take effect the Framebuffer object must be recreated using the `OnUpdate()` function
        void     SetFramebufferSize(float width, float height);
        const glm::vec2& GetFramebufferSize() { return M_FramebufferSize; };
        /// Returns the opengl texture Id of texture made using the Framebuffer object
        uint32_t GetColorAttachmentId() const { return M_ColorAttachmentId; };
        /// Binds the Framebuffer object
        void     Bind() const;
        /// Unbinds the Framebuffer object
        void     Unbind() const;
    private:
        /// Renderer Ids, for the Framebuffer object, the texture of color attachment and for the depth attachment
        uint32_t  M_FramebufferId, M_ColorAttachmentId, M_DepthAttachmentId;
        /// Used by the `OnUpdate()` function to recreate the Framebuffer object
        glm::vec2 M_FramebufferSize;
    };
}