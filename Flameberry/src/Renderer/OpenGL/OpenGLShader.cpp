#include "OpenGLShader.h"

#include <glad/glad.h>

#include "Core/Core.h"
#include "Core/Timer.h"
#include "Core/FileHandler.h"

#include "OpenGLRenderCommand.h"

namespace Flameberry {
    OpenGLShader::OpenGLShader(const std::string& filePath, const std::vector<OpenGLShaderBinding>& bindings)
        : m_ProgramID(0)
    {
        CreateShaderFromSingleFile(filePath);
        for (const auto& binding : bindings)
        {
            uint32_t blockIndex = glGetUniformBlockIndex(m_ProgramID, binding.blockName.c_str());
            glUniformBlockBinding(m_ProgramID, blockIndex, binding.blockBindingIndex);
        }
    }

    OpenGLShader::OpenGLShader(const std::string& vertexPath, const std::string& fragmentPath, const std::vector<OpenGLShaderBinding>& bindings)
    {
#if 1
        CreateShader(vertexPath.c_str(), fragmentPath.c_str());
#else
        CreateShaderWithFStream(vertexPath.c_str(), fragmentPath.c_str());
#endif

        for (const auto& binding : bindings)
        {
            uint32_t blockIndex = glGetUniformBlockIndex(m_ProgramID, binding.blockName.c_str());
            glUniformBlockBinding(m_ProgramID, blockIndex, binding.blockBindingIndex);
        }
    }

    OpenGLShader::~OpenGLShader()
    {
        glDeleteProgram(m_ProgramID);
    }

    void OpenGLShader::Bind()
    {
        glUseProgram(m_ProgramID);
    }

    void OpenGLShader::Unbind()
    {
        glUseProgram(0);
    }

    void OpenGLShader::PushUniformInt(const std::string& uniformName, int value)
    {
        glUniform1i(GetUniformLocation(uniformName), value);
    }

    void OpenGLShader::PushUniformIntArray(const std::string& uniformName, uint32_t count, const int* values)
    {
        glUniform1iv(GetUniformLocation(uniformName), count, values);
    }

    void OpenGLShader::PushUniformFloat(const std::string& uniformName, float value)
    {
        glUniform1f(GetUniformLocation(uniformName), value);
    }

    void OpenGLShader::PushUniformFloat3(const std::string& uniformName, float v0, float v1, float v2)
    {
        glUniform3f(GetUniformLocation(uniformName), v0, v1, v2);
    }

    void OpenGLShader::PushUniformFloat4(const std::string& uniformName, float v0, float v1, float v2, float v3)
    {
        glUniform4f(GetUniformLocation(uniformName), v0, v1, v2, v3);
    }

    void OpenGLShader::PushUniformMatrix4f(const std::string& uniformName, uint32_t count, bool transpose, const float* value)
    {
        glUniformMatrix4fv(GetUniformLocation(uniformName), count, transpose, value);
    }

    int OpenGLShader::GetUniformLocation(const std::string& uniformName)
    {
        if (m_UniformLocationCache.find(uniformName) != m_UniformLocationCache.end() && m_UniformLocationCache[uniformName] != -1)
            return m_UniformLocationCache[uniformName];
        GLint location = glGetUniformLocation(m_ProgramID, uniformName.c_str());
        m_UniformLocationCache[uniformName] = location;
        return location;
    }

    void OpenGLShader::CreateShaderFromSingleFile(const std::string& shaderFilePath)
    {
        FL_LOG(shaderFilePath);
        FL_SCOPED_TIMER("fstream_create_shader_single_file");

        auto [vertexSource, fragmentSource] = OpenGLRenderCommand::ReadShaderSource(shaderFilePath);
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
        m_ProgramID = glCreateProgram();

        // Attach our shaders to our program
        glAttachShader(m_ProgramID, vertex_shader);
        glAttachShader(m_ProgramID, fragment_shader);

        // Link our program
        glLinkProgram(m_ProgramID);

        // Note the different functions here: glGetProgram* instead of glGetShader*.
        GLint isLinked = 0;
        glGetProgramiv(m_ProgramID, GL_LINK_STATUS, (int*)&isLinked);
        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(m_ProgramID, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(m_ProgramID, maxLength, &maxLength, &infoLog[0]);

            // We don't need the program anymore.
            glDeleteProgram(m_ProgramID);
            // Don't leak shaders either.
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);

            FL_ERROR("Error linking shader program:\n{0}", infoLog.data());
        }

        // Always detach shaders after a successful link.
        glDetachShader(m_ProgramID, vertex_shader);
        glDetachShader(m_ProgramID, fragment_shader);
    }

    void OpenGLShader::CreateShader(const char* vertex, const char* fragment)
    {
        FL_LOG(vertex);
        FL_SCOPED_TIMER("mmap_create_shader");

        // Create an empty vertex shader handle
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

        FileHandler vertexHandler(vertex);
        char* vertexSrc = vertexHandler.MapFileToMemory();

        // Send the vertex shader source code to GL
        // Note that std::string's .c_str is NULL character terminated.
        const GLchar* source = (const GLchar*)vertexSrc;
        glShaderSource(vertex_shader, 1, &source, 0);

        vertexHandler.UnmapFileFromMemory();

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

        FileHandler fragmentHandler(fragment);
        char* fragmentSrc = fragmentHandler.MapFileToMemory();

        // Send the fragment shader source code to GL
        // Note that std::string's .c_str is NULL character terminated.
        source = (const GLchar*)fragmentSrc;
        glShaderSource(fragment_shader, 1, &source, 0);

        fragmentHandler.UnmapFileFromMemory();

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
        m_ProgramID = glCreateProgram();

        // Attach our shaders to our program
        glAttachShader(m_ProgramID, vertex_shader);
        glAttachShader(m_ProgramID, fragment_shader);

        // Link our program
        glLinkProgram(m_ProgramID);

        // Note the different functions here: glGetProgram* instead of glGetShader*.
        GLint isLinked = 0;
        glGetProgramiv(m_ProgramID, GL_LINK_STATUS, (int*)&isLinked);
        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(m_ProgramID, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(m_ProgramID, maxLength, &maxLength, &infoLog[0]);

            // We don't need the program anymore.
            glDeleteProgram(m_ProgramID);
            // Don't leak shaders either.
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);

            FL_ERROR("Error linking shader program:\n{0}", infoLog.data());
        }

        // Always detach shaders after a successful link.
        glDetachShader(m_ProgramID, vertex_shader);
        glDetachShader(m_ProgramID, fragment_shader);
    }

    void OpenGLShader::CreateShaderWithFStream(const char* vertex, const char* fragment)
    {
        FL_LOG(vertex);
        FL_SCOPED_TIMER("fstream_create_shader");

        auto [vertexSource, fragmentSource] = OpenGLRenderCommand::ReadShaderSource(vertex, fragment);

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
        m_ProgramID = glCreateProgram();

        // Attach our shaders to our program
        glAttachShader(m_ProgramID, vertex_shader);
        glAttachShader(m_ProgramID, fragment_shader);

        // Link our program
        glLinkProgram(m_ProgramID);

        // Note the different functions here: glGetProgram* instead of glGetShader*.
        GLint isLinked = 0;
        glGetProgramiv(m_ProgramID, GL_LINK_STATUS, (int*)&isLinked);
        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(m_ProgramID, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(m_ProgramID, maxLength, &maxLength, &infoLog[0]);

            // We don't need the program anymore.
            glDeleteProgram(m_ProgramID);
            // Don't leak shaders either.
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);

            FL_ERROR("Error linking shader program:\n{0}", infoLog.data());
        }

        // Always detach shaders after a successful link.
        glDetachShader(m_ProgramID, vertex_shader);
        glDetachShader(m_ProgramID, fragment_shader);
    }
}