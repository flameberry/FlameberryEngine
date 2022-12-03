#include "OpenGLUniformBuffer.h"

#include <glad/glad.h>

namespace Flameberry {
    UniformBuffer::UniformBuffer(size_t size, uint32_t bindingPoint)
    {
        glGenBuffers(1, &m_RendererID);
        glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
        glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, m_RendererID, 0, size);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void UniformBuffer::Bind()
    {
        glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
    }

    void UniformBuffer::Unbind()
    {
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void UniformBuffer::SetData(const void* data, size_t size, size_t offset)
    {
        glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
    }

    UniformBuffer::~UniformBuffer()
    {
        glDeleteBuffers(1, &m_RendererID);
    }
}

