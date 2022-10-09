#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Renderer/PerspectiveCamera.h"
#include "OpenGLVertex.h"

namespace Flameberry {
    struct OpenGLVertex
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 texture_uv;
        float texture_index;

        /// Default Constructor
        OpenGLVertex()
            : position(0.0f), color(1.0f), texture_uv(0.0f), texture_index(-1.0f)
        {
        }
    };

    class OpenGLRenderer3D
    {
    public:
        static std::shared_ptr<OpenGLRenderer3D> Create();
        void Init(GLFWwindow* window);
        void CleanUp();
        void Begin(const PerspectiveCamera& camera);
        void End();
        void OnDraw();
        float GetAspectRatio() const { return m_AspectRatio; };
        GLint GetUniformLocation(const std::string& name, uint32_t shaderId);
    private:
        void UpdateViewportSize();
    private:
        struct UniformBufferData { glm::mat4 ModelViewProjectionMatrix; };
    private:
        GLFWwindow* m_UserGLFWwindow;
        uint32_t m_VertexArrayId, m_VertexBufferId, m_IndexBufferId, m_ShaderProgramId, m_UniformBufferId, m_TextureId;
        std::unordered_map<std::string, GLint> m_UniformLocationCache;
        UniformBufferData m_UniformBufferData;
        float m_AspectRatio = 1280.0f / 720.0f;
        glm::vec2 m_ViewportSize;

        std::vector<OpenGLVertex> m_TempVertices;
        std::vector<uint32_t> m_TempIndices;
    };
}
