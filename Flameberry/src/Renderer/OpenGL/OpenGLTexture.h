#pragma once

#include <stdint.h>
#include <unordered_map>
#include <memory>

namespace Flameberry {
    class OpenGLTexture
    {
    public:
        OpenGLTexture(const std::string& filePath);
        ~OpenGLTexture();

        void BindTextureUnit(uint32_t unit);
        void Unbind();

        inline uint32_t GetTextureID() const { return m_TextureID; }

        template<typename... Args>
        static std::shared_ptr<OpenGLTexture> Create(Args... args) { return std::make_shared<OpenGLTexture>(std::forward<Args>(args)...); }
    private:
        uint32_t m_TextureID;
    };

}