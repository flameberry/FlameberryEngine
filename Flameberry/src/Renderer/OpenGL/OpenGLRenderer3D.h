#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Renderer/PerspectiveCamera.h"
#include "Renderer/Mesh.h"
#include "OpenGLUniformBuffer.h"

#include "Renderer/Light.h"

#define FL_UNIFORM_BLOCK_BINDING_CAMERA 0
#define FL_UNIFORM_BLOCK_BINDING_LIGHTING 1

#define FL_MAX_POINT_LIGHTS 10

namespace Flameberry {
    struct SceneData;

    class OpenGLRenderer3D
    {
    public:
        OpenGLRenderer3D();
        ~OpenGLRenderer3D() = default;
        static std::shared_ptr<OpenGLRenderer3D> Create();

        void Init(GLFWwindow* window);
        void CleanUp();
        void Begin(const PerspectiveCamera& camera);
        void End();

        float GetAspectRatio() const { return m_AspectRatio; };
        GLint GetUniformLocation(const std::string& name, uint32_t shaderId);
    private:
        void UpdateViewportSize();
    private:
        struct CameraUniformBufferData { glm::mat4 ViewProjectionMatrix; };
    private:
        GLFWwindow* m_UserGLFWwindow;
        uint32_t m_VertexArrayId, m_VertexBufferId, m_IndexBufferId, m_ShaderProgramId, m_TextureId;
        CameraUniformBufferData m_UniformBufferData;
        float m_AspectRatio = 1280.0f / 720.0f;
        glm::vec2 m_ViewportSize;

        UniformBuffer m_CameraUniformBuffer;
    };
}
