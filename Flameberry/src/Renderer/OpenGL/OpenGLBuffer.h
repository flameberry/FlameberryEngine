#pragma once

#include <stdio.h>
#include <glad/glad.h>

namespace Flameberry {
    class OpenGLBuffer
    {
    public:
        OpenGLBuffer(uint32_t size, const void* data, GLenum bufferTarget, GLenum bufferUsage);
        ~OpenGLBuffer();

        void BufferSubData(const void* data, size_t size, size_t offset);
        void BindBufferBase(uint32_t index);

        void Bind();
        void Unbind();
    private:
        uint32_t m_BufferID;
        GLenum m_BufferTarget;
    };
}
