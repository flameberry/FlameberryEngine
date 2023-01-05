#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Flameberry {
    struct OpenGLShaderBinding
    {
        uint32_t blockBindingIndex;
        std::string blockName;
    };

    class OpenGLShader
    {
    public:
        OpenGLShader(const std::string& filePath, const std::vector<OpenGLShaderBinding>& bindings = {});
        ~OpenGLShader();

        void Bind();
        void Unbind();

        void PushUniformInt(const std::string& uniformName, int value);
        void PushUniformFloat(const std::string& uniformName, float value);
        void PushUniformFloat3(const std::string& uniformName, float v0, float v1, float v2);
        void PushUniformFloat4(const std::string& uniformName, float v0, float v1, float v2, float v3);
        void PushUniformMatrix4f(const std::string& uniformName, uint32_t count, bool transpose, const float* value);

        static std::shared_ptr<OpenGLShader> Create(const std::string& filePath, const std::vector<OpenGLShaderBinding>& bindings = {})
        {
            return std::make_shared<OpenGLShader>(filePath, bindings);
        }
    private:
        int GetUniformLocation(const std::string& uniformName);
    private:
        uint32_t m_ProgramID;
        std::unordered_map<std::string, int> m_UniformLocationCache;
    };
}
