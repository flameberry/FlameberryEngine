#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Flameberry {
    class RenderCommand
    {
    public:
        static std::tuple<std::string, std::string> ReadShaderSource(const std::string& filePath);
        static uint32_t CreateTexture(const std::string& filePath);
        static uint32_t CreateShader(const std::string& filePath);
        static glm::vec3 PixelToOpenGL(const glm::vec3& coords, const glm::vec2& viewportSize, float zNear, float zFar);
        static glm::vec2 PixelToOpenGL(const glm::vec2& coords, const glm::vec2& viewportSize);
        static float PixelToOpenGLX(float pixels, const glm::vec2& viewportSize);
        static float PixelToOpenGLY(float pixels, float viewportHeight);
    };
}