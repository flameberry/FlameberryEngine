#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Renderer/PerspectiveCamera.h"
#include "Renderer/Mesh.h"

namespace Flameberry {
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
        struct UniformBufferData { glm::mat4 ViewProjectionMatrix; };
    private:
        GLFWwindow* m_UserGLFWwindow;
        uint32_t m_VertexArrayId, m_VertexBufferId, m_IndexBufferId, m_ShaderProgramId, m_UniformBufferId, m_UniformBufferId_2, m_TextureId;
        UniformBufferData m_UniformBufferData;
        float m_AspectRatio = 1280.0f / 720.0f;
        glm::vec2 m_ViewportSize;
    };
}
