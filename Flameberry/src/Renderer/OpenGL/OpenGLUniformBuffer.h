#pragma once

#include <iostream>

namespace Flameberry {
    class UniformBuffer
    {
    public:
        UniformBuffer(size_t size, uint32_t bindingPoint = 0);
        ~UniformBuffer();

        void Bind();
        void Unbind();
        void SetData(const void* data, size_t size, size_t offset);
    private:
        uint32_t m_RendererID;
    };
}