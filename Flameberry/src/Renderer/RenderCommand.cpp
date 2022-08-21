#include "RenderCommand.h"

#include <fstream>
#include "Core/Core.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

namespace utils {
    float map(float value, const glm::vec2& initial_range, const glm::vec2& target_range)
    {
        return target_range.x + (value - initial_range.x) * (target_range.y - target_range.x) / (initial_range.y - initial_range.x);
    }
}

namespace Flameberry {
    glm::vec3 RenderCommand::PixelToOpenGL(const glm::vec3& coords, const glm::vec2& viewportSize, float zNear, float zFar)
    {
        float aspectRatio = viewportSize.x / viewportSize.y;
        float x = utils::map(coords.x, { -viewportSize.x / 2.0f , viewportSize.x / 2.0f }, { -aspectRatio, aspectRatio });
        float y = utils::map(coords.y, { -viewportSize.y / 2.0f , viewportSize.y / 2.0f }, { -1.0f, 1.0f });
        // Suspicious z
        float z = utils::map(coords.z, { 0.0f, 1000.0f }, { zNear, zFar });
        return glm::vec3(x, y, z);
    }

    glm::vec2 RenderCommand::PixelToOpenGL(const glm::vec2& coords, const glm::vec2& viewportSize)
    {
        float aspectRatio = viewportSize.x / viewportSize.y;
        float x = utils::map(coords.x, { -viewportSize.x / 2.0f , viewportSize.x / 2.0f }, { -aspectRatio, aspectRatio });
        float y = utils::map(coords.y, { -viewportSize.y / 2.0f , viewportSize.y / 2.0f }, { -1.0f, 1.0f });
        return glm::vec2(x, y);
    }

    float RenderCommand::PixelToOpenGLX(float pixels, const glm::vec2& viewportSize)
    {
        float aspectRatio = viewportSize.x / viewportSize.y;
        return utils::map(pixels, { -viewportSize.x / 2.0f, viewportSize.x / 2.0f }, { -aspectRatio, aspectRatio });
    }

    float RenderCommand::PixelToOpenGLY(float pixels, float viewportHeight)
    {
        return utils::map(pixels, { -viewportHeight / 2.0f, viewportHeight / 2.0f }, { -1.0f, 1.0f });
    }

    std::tuple<std::string, std::string> RenderCommand::ReadShaderSource(const std::string& filePath)
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

    uint32_t RenderCommand::CreateShader(const std::string& filePath)
    {
        uint32_t shaderProgramId = 0;
        auto [vertexSource, fragmentSource] = RenderCommand::ReadShaderSource(filePath);
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

    uint32_t RenderCommand::CreateTexture(const std::string& filePath)
    {
        stbi_set_flip_vertically_on_load(true);

        int width, height, channels;
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);

        FL_DO_ON_ASSERT(data, FL_ERROR("Failed to load texture from \"{0}\"", filePath));

        GLenum internalFormat = 0, dataFormat = 0;
        if (channels == 4)
        {
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
        }
        else if (channels == 3)
        {
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
        }

        uint32_t textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        return textureId;
    }
}