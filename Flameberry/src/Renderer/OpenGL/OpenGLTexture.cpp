#include "OpenGLTexture.h"

#include <glad/glad.h>
#include <stb_image/stb_image.h>

#include "OpenGLRenderCommand.h"
#include "Core/Core.h"

namespace Flameberry {
    OpenGLTextureSpecification::OpenGLTextureSpecification()
        : SetupTextureProperties([](uint32_t target)
            {
                glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            })
    {}

    OpenGLTexture::OpenGLTexture(const std::string& filePath)
        : m_TextureID(OpenGLRenderCommand::GetTextureIdIfAvailable(filePath.c_str())), m_FilePath(filePath)
    {
        if (m_TextureID) return;
        LoadTexture2DFromFile(filePath);
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
        glBindTexture(m_Specifications.Target, m_TextureID);
    }

    void OpenGLTexture::Unbind()
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void OpenGLTexture::LoadTexture2DFromFile(const std::string& filePath)
    {
        m_FilePath = filePath;
        m_Specifications.Target = GL_TEXTURE_2D;

        stbi_set_flip_vertically_on_load(true);
        int channels;
        unsigned char* data = stbi_load(filePath.c_str(), &m_Specifications.Width, &m_Specifications.Height, &channels, 0);

        FL_DO_ON_ASSERT(data, FL_ERROR("Failed to load texture from \"{0}\"", filePath));

        // GLenum internalFormat = 0, dataFormat = 0;
        switch (channels)
        {
        case 4: {
            m_Specifications.InternalFormat = GL_RGBA8;
            m_Specifications.Format = GL_RGBA;
            break;
        }
        case 3: {
            m_Specifications.InternalFormat = GL_RGB8;
            m_Specifications.Format = GL_RGB;
            break;
        }
        case 1: {
            m_Specifications.InternalFormat = GL_RGBA;
            m_Specifications.Format = GL_RED;
            break;
        }
        }

        glGenTextures(1, &m_TextureID);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);

        m_Specifications.SetupTextureProperties(GL_TEXTURE_2D);

        glTexImage2D(GL_TEXTURE_2D, 0, m_Specifications.InternalFormat, m_Specifications.Width, m_Specifications.Height, 0, m_Specifications.Format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        OpenGLRenderCommand::AddTextureIDToCache(m_FilePath, m_TextureID);
    }

    void OpenGLTexture::LoadCubeMapFromFolder(const std::string& folderPath)
    {
        m_Specifications.Target = GL_TEXTURE_CUBE_MAP;
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

            if (width != m_Specifications.Width || height != m_Specifications.Height)
                FL_ERROR("Failed to load cubemap: Dimensions of all faces must be same");

            m_Specifications.Width = width;
            m_Specifications.Height = height;

            stbi_set_flip_vertically_on_load(false);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
}
