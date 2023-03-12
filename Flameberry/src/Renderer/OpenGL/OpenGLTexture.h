#pragma once

#include <stdint.h>
#include <unordered_map>
#include <memory>
#include <string>

#include <glad/glad.h>

namespace Flameberry {
    class OpenGLTexture
    {
    public:
        OpenGLTexture(const std::string& texturePath, uint32_t minFilter = GL_LINEAR, uint32_t magFilter = GL_LINEAR);
        ~OpenGLTexture();

        void BindTextureUnit(uint32_t unit);
        void Unbind();

        inline uint32_t GetTextureID() const { return m_TextureID; }
        inline std::string GetFilePath() const { return m_FilePath; }

        template<typename... Args>
        static std::shared_ptr<OpenGLTexture> Create(Args... args) { return std::make_shared<OpenGLTexture>(std::forward<Args>(args)...); }

        void LoadTexture2DFromFile(const std::string& filePath);
        void LoadCubeMapFromFolder(const std::string& folderPath);
    private:
        std::string m_FilePath;
        uint32_t m_TextureID;

        int m_Width, m_Height;
        uint32_t m_Target, m_InternalFormat, m_Format, m_MinFilter, m_MagFilter;
    };

}
