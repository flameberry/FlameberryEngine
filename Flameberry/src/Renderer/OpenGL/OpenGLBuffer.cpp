#include "OpenGLBuffer.h"

#include <glad/glad.h>

namespace Flameberry {
    OpenGLBuffer::OpenGLBuffer(uint32_t size, const void* data, GLenum bufferTarget, GLenum bufferUsage)
        : m_BufferID(0), m_BufferTarget(bufferTarget)
    {
        glGenBuffers(1, &m_BufferID);
        glBindBuffer(bufferTarget, m_BufferID);
        glBufferData(bufferTarget, size, data, bufferUsage);
    }

    OpenGLBuffer::~OpenGLBuffer()
    {
        glDeleteBuffers(1, &m_BufferID);
    }

    void OpenGLBuffer::BufferSubData(const void* data, size_t size, size_t offset)
    {
        glBufferSubData(m_BufferTarget, offset, size, data);
    }

    void OpenGLBuffer::BindBufferBase(uint32_t index)
    {
        glBindBufferBase(m_BufferTarget, index, m_BufferID);
    }

    void OpenGLBuffer::Bind()
    {
        glBindBuffer(m_BufferTarget, m_BufferID);
    }

    void OpenGLBuffer::Unbind()
    {
        glBindBuffer(m_BufferTarget, 0);
    }
}