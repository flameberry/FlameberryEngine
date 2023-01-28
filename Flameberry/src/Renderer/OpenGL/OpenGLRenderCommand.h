#pragma once

#include <string>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <utility>
#include <glad/glad.h>

#include "OpenGLVertex.h"

namespace Flameberry {
    // A hash function used to hash a pair of any kind
    struct hash_pair {
        template <class T1, class T2>
        size_t operator()(const std::pair<T1, T2>& p) const
        {
            auto hash1 = std::hash<T1>{}(p.first);
            auto hash2 = std::hash<T2>{}(p.second);

            if (hash1 != hash2) {
                return hash1 ^ hash2;
            }

            // If hash1 == hash2, their XOR is zero.
            return hash1;
        }
    };

    struct ModelData {
        std::vector<OpenGLVertex2D> Vertices;
        std::vector<uint32_t> Indices;
    };

    class OpenGLRenderCommand
    {
    public:
        static std::tuple<std::string, std::string> ReadShaderSource(const std::string& filePath);
        static std::tuple<std::string, std::string> ReadShaderSource(const std::string& vertexPath, const std::string& fragmentPath);
        static uint32_t CreateTexture(const std::string& filePath);
        static uint32_t CreateCubeMap(const char* folderPath);
        static uint32_t CreateShader(const std::string& filePath);
        static std::tuple<std::vector<OpenGLVertex>, std::vector<uint32_t>> LoadModel(const std::string& filePath);
        static uint32_t GetUniformLocation(uint32_t shaderProgramID, const std::string& uniformName);

        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        static void EnableBlend(GLenum sFactor = GL_SRC_ALPHA, GLenum dFactor = GL_ONE_MINUS_SRC_ALPHA);
        static void EnableDepthTest(GLenum depthFunc = GL_LESS);
        static uint32_t GetTextureIDIfAvailable(const char* textureFilePath);
        static void AddTextureIDToCache(const std::string& textureFilePath, uint32_t textureID) { s_TextureIdCache[textureFilePath] = textureID; }
        static void RemoveTextureIDFromCache(const std::string& textureFilePath) { s_TextureIdCache.erase(textureFilePath); }
    private:
        // Stores the texture IDs of the already loaded textures to be reused
        static std::unordered_map<std::string, uint32_t> s_TextureIdCache;
        static std::unordered_map<std::pair<std::string, uint32_t>, uint32_t, hash_pair> s_UniformLocationCache;
    };
}
