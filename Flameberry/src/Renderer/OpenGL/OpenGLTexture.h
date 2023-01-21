#pragma once

#include <stdint.h>
#include <unordered_map>
#include <memory>
#include <string>

namespace Flameberry {
    struct OpenGLTextureSpecification {
        int Width, Height;
        uint32_t Target, InternalFormat, Format;
        void(*SetupTextureProperties)(uint32_t target);

        OpenGLTextureSpecification();
    };

    class OpenGLTexture
    {
    public:
        OpenGLTexture(const std::string& filePath);
        OpenGLTexture(uint32_t textureID);
        ~OpenGLTexture();

        void BindTextureUnit(uint32_t unit);
        void Unbind();

        inline uint32_t GetTextureID() const { return m_TextureID; }

        template<typename... Args>
        static std::shared_ptr<OpenGLTexture> Create(Args... args) { return std::make_shared<OpenGLTexture>(std::forward<Args>(args)...); }

        void LoadTexture2DFromFile(const std::string& filePath);
        void LoadCubeMapFromFolder(const std::string& folderPath);
    private:
        std::string m_FilePath;
        uint32_t m_TextureID;

        OpenGLTextureSpecification m_Specifications;
    };

}
