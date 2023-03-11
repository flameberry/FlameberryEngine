#include "OpenGLRenderCommand.h"

#include <fstream>
#include <glad/glad.h>

#include "Core/Core.h"
#include "Core/Timer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

namespace Flameberry {
    std::unordered_map<std::string, uint32_t> OpenGLRenderCommand::s_TextureIdCache;
    std::unordered_map<std::pair<std::string, uint32_t>, uint32_t, hash_pair> OpenGLRenderCommand::s_UniformLocationCache;

    void OpenGLRenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        glViewport(x, y, width, height);
    }

    void OpenGLRenderCommand::EnableBlend(GLenum sFactor, GLenum dFactor)
    {
        glEnable(GL_BLEND);
        glBlendFunc(sFactor, dFactor);
    }

    void OpenGLRenderCommand::EnableDepthTest(GLenum depthFunc)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(depthFunc);
    }

    std::tuple<std::vector<OpenGLVertex>, std::vector<uint32_t>> OpenGLRenderCommand::LoadModel(const std::string& filePath)
    {
        FL_SCOPED_TIMER("Load_Model_tiny");
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        std::vector<OpenGLVertex> vertices;
        std::vector<uint32_t> indices;

        FL_ASSERT(tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str()), err);

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                OpenGLVertex vertex{};

                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texture_uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };

                vertex.color = { 1.0f, 1.0f, 1.0f, 1.0f };

                vertices.push_back(vertex);
                indices.push_back(indices.size());
            }
        }
        return { vertices, indices };
    }

    std::tuple<std::string, std::string> OpenGLRenderCommand::ReadShaderSource(const std::string& filePath)
    {
        std::ifstream stream(filePath);

        FL_ASSERT(stream.is_open(), "The given shader file {0} cannot be opened", filePath);

        std::stringstream ss[2];
        std::string line;

        uint32_t shader_type = 2;
        while (getline(stream, line))
        {
            if (line.find("#shader") != std::string::npos)
            {
                if (line.find("vertex") != std::string::npos)
                    shader_type = 0;
                else if (line.find("fragment") != std::string::npos)
                    shader_type = 1;
            }
            else
            {
                ss[shader_type] << line << "\n";
            }
        }
        stream.close();
        return std::make_tuple(ss[0].str(), ss[1].str());
    }

    std::tuple<std::string, std::string> OpenGLRenderCommand::ReadShaderSource(const std::string& vertexPath, const std::string& fragmentPath)
    {
        std::stringstream ss[2];
        std::string line;

        std::ifstream stream(vertexPath);
        FL_ASSERT(stream.is_open(), "The given shader file {0} cannot be opened", vertexPath);

        while (getline(stream, line))
            ss[0] << line << "\n";
        stream.close();

        std::ifstream fragmentStream(fragmentPath);
        FL_ASSERT(fragmentStream.is_open(), "The given shader file {0} cannot be opened", vertexPath);

        while (getline(fragmentStream, line))
            ss[1] << line << "\n";
        fragmentStream.close();

        return std::make_tuple(ss[0].str(), ss[1].str());
    }

    uint32_t OpenGLRenderCommand::GetUniformLocation(uint32_t shaderProgramID, const std::string& uniformName)
    {
        std::pair<std::string, uint32_t> key = { uniformName, shaderProgramID };
        if (s_UniformLocationCache.find(key) != s_UniformLocationCache.end())
            return s_UniformLocationCache[key];

        GLint location = glGetUniformLocation(shaderProgramID, uniformName.c_str());
        if (location == -1)
            FL_WARN("Uniform \"{0}\" not found!", uniformName);
        s_UniformLocationCache[key] = location;
        return location;
    }

    uint32_t OpenGLRenderCommand::CreateShader(const std::string& filePath)
    {
        uint32_t shaderProgramId = 0;
        auto [vertexSource, fragmentSource] = OpenGLRenderCommand::ReadShaderSource(filePath);
        // Create an empty vertex shader handle
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

        // Send the vertex shader source code to GL
        // Note that std::string's .c_str is NULL character terminated.
        const GLchar* source = (const GLchar*)vertexSource.c_str();
        glShaderSource(vertex_shader, 1, &source, 0);

        // Compile the vertex shader
        glCompileShader(vertex_shader);

        GLint isCompiled = 0;
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(vertex_shader, maxLength, &maxLength, &infoLog[0]);

            // We don't need the shader anymore.
            glDeleteShader(vertex_shader);

            FL_ERROR("Error compiling VERTEX shader:\n{0}", infoLog.data());
        }

        // Create an empty fragment shader handle
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

        // Send the fragment shader source code to GL
        // Note that std::string's .c_str is NULL character terminated.
        source = (const GLchar*)fragmentSource.c_str();
        glShaderSource(fragment_shader, 1, &source, 0);

        // Compile the fragment shader
        glCompileShader(fragment_shader);

        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(fragment_shader, maxLength, &maxLength, &infoLog[0]);

            // We don't need the shader anymore.
            glDeleteShader(fragment_shader);
            // Either of them. Don't leak shaders.
            glDeleteShader(vertex_shader);

            FL_ERROR("Error compiling FRAGMENT shader:\n{0}", infoLog.data());
        }

        // Vertex and fragment shaders are successfully compiled.
        // Now time to link them together into a program.
        // Get a program object.
        shaderProgramId = glCreateProgram();

        // Attach our shaders to our program
        glAttachShader(shaderProgramId, vertex_shader);
        glAttachShader(shaderProgramId, fragment_shader);

        // Link our program
        glLinkProgram(shaderProgramId);

        // Note the different functions here: glGetProgram* instead of glGetShader*.
        GLint isLinked = 0;
        glGetProgramiv(shaderProgramId, GL_LINK_STATUS, (int*)&isLinked);
        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(shaderProgramId, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(shaderProgramId, maxLength, &maxLength, &infoLog[0]);

            // We don't need the program anymore.
            glDeleteProgram(shaderProgramId);
            // Don't leak shaders either.
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);

            FL_ERROR("Error linking shader program:\n{0}", infoLog.data());
        }

        // Always detach shaders after a successful link.
        glDetachShader(shaderProgramId, vertex_shader);
        glDetachShader(shaderProgramId, fragment_shader);

        return shaderProgramId;
    }

    uint32_t OpenGLRenderCommand::GetTextureIDIfAvailable(const char* textureFilePath)
    {
        if (s_TextureIdCache.find(textureFilePath) != s_TextureIdCache.end())
            return s_TextureIdCache[textureFilePath];
        else
            return 0;
    }

    uint32_t OpenGLRenderCommand::CreateTexture(const std::string& filePath)
    {
        uint32_t textureID = GetTextureIDIfAvailable(filePath.c_str());
        if (textureID)
            return textureID;

        stbi_set_flip_vertically_on_load(true);

        int width, height, channels;
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);

        FL_DO_ON_ASSERT(data, FL_ERROR("Failed to load texture from \"{0}\"", filePath));

        GLenum internalFormat = 0, dataFormat = 0;
        switch (channels)
        {
        case 4: {
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
            break;
        }
        case 3: {
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
            break;
        }
        case 1: {
            internalFormat = GL_RGBA;
            dataFormat = GL_RED;
            break;
        }
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        s_TextureIdCache[filePath] = textureID;

        return textureID;
    }

    uint32_t OpenGLRenderCommand::CreateCubeMap(const char* folderPath)
    {
        uint32_t textureID = 0;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

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
            FL_ASSERT(data, "Failed to load cube map face: {0}", paths[i]);
            stbi_set_flip_vertically_on_load(false);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return textureID;
    }
}
