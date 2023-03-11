#include "OpenGLTexture.h"

#include <filesystem>
#include <glad/glad.h>
#include <stb_image/stb_image.h>

#include "OpenGLRenderCommand.h"
#include "Core/Core.h"

namespace Flameberry {
    OpenGLTexture::OpenGLTexture(const std::string& texturePath, uint32_t minFilter, uint32_t magFilter)
        : m_TextureID(OpenGLRenderCommand::GetTextureIDIfAvailable(texturePath.c_str())), m_FilePath(texturePath),
        m_MinFilter(minFilter), m_MagFilter(magFilter)
    {
        if (m_TextureID) return;

        std::filesystem::path tPath(texturePath);
        if (!tPath.has_extension())
            LoadCubeMapFromFolder(texturePath);
        else
            LoadTexture2DFromFile(texturePath);
    }

    OpenGLTexture::OpenGLTexture(uint32_t textureID)
        : m_TextureID(textureID)
    {}

    OpenGLTexture::~OpenGLTexture()
    {
        OpenGLRenderCommand::RemoveTextureIDFromCache(m_FilePath);
        // glDeleteTextures(1, &m_TextureID); // TODO: Don't know why this causes problems
    }

    void OpenGLTexture::BindTextureUnit(uint32_t unit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(m_Target, m_TextureID);
    }

    void OpenGLTexture::Unbind()
    {
        glBindTexture(m_Target, 0);
    }

    void OpenGLTexture::LoadTexture2DFromFile(const std::string& filePath)
    {
        m_FilePath = filePath;
        m_Target = GL_TEXTURE_2D;

        stbi_set_flip_vertically_on_load(true);
        int channels;
        unsigned char* data = stbi_load(filePath.c_str(), &m_Width, &m_Height, &channels, 0);

        FL_DO_ON_ASSERT(data, FL_ERROR("Failed to load texture from \"{0}\"", filePath));

        switch (channels)
        {
        case 4: {
            m_InternalFormat = GL_RGBA8;
            m_Format = GL_RGBA;
            break;
        }
        case 3: {
            m_InternalFormat = GL_RGB8;
            m_Format = GL_RGB;
            break;
        }
        case 1: {
            m_InternalFormat = GL_RGBA;
            m_Format = GL_RED;
            break;
        }
        }

        glGenTextures(1, &m_TextureID);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);

        glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, m_MinFilter);
        glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, m_MagFilter);


        glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_Format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        OpenGLRenderCommand::AddTextureIDToCache(m_FilePath, m_TextureID);
    }

    void OpenGLTexture::LoadCubeMapFromFolder(const std::string& folderPath)
    {
        m_Target = GL_TEXTURE_CUBE_MAP;
        m_InternalFormat = GL_RGB8;
        m_Format = GL_RGB;

        glGenTextures(1, &m_TextureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);

        std::string paths[] = {
             std::string(folderPath) + "/right.jpg",
             std::string(folderPath) + "/left.jpg",
             std::string(folderPath) + "/top.jpg",
             std::string(folderPath) + "/bottom.jpg",
             std::string(folderPath) + "/front.jpg",
             std::string(folderPath) + "/back.jpg"
        };

        for (uint32_t i = 0; i < 6; i++)
        {
            int width, height, channels;
            unsigned char* data = stbi_load(paths[i].c_str(), &width, &height, &channels, 0);
            FL_ASSERT(data, "Failed to load cube maps face: {0}", paths[i]);

            if (width != m_Width || height != m_Height)
                FL_ERROR("Failed to load cubemap: Dimensions of all faces must be same");

            m_Width = width;
            m_Height = height;

            stbi_set_flip_vertically_on_load(false);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
}
