#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "OpenGLVertex.h"

#include "Core/Core.h"
#include "Renderer/OrthographicCamera.h"

// This Macro contains the max number of texture slots that the GPU supports, varies for each computer.
#define MAX_TEXTURE_SLOTS 16
#define MAX_QUADS 1000
#define MAX_VERTICES 4 * MAX_QUADS
#define MAX_INDICES 6 * MAX_QUADS

namespace Flameberry {
    struct OpenGLRenderer2DInitInfo
    {
        GLFWwindow* userWindow;
        bool enableCustomViewport{ false };
        glm::vec2 customViewportSize;
    };

    class OpenGLRenderer2D
    {
    public:
        OpenGLRenderer2D();
        ~OpenGLRenderer2D();

        void        Init(const OpenGLRenderer2DInitInfo& rendererInitInfo);
        GLFWwindow* GetUserGLFWwindow();
        glm::vec2   GetViewportSize();
        GLint       GetUniformLocation(const std::string& name, uint32_t shaderId);
        void        AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const char* textureFilePath);
        void        AddQuad(const glm::mat4& transform, const glm::vec4& color, uint32_t entityID);
        void        AddQuad(const glm::mat4& transform, const char* textureFilePath, uint32_t entityID);
        void        AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const glm::vec4& color);
        void        CleanUp();
        void        Begin(const OrthographicCamera& camera);
        void        End();
        void        SetCustomViewportSize(const glm::vec2& customViewportSize) { m_RendererInitInfo.customViewportSize = customViewportSize; }
        static std::shared_ptr<OpenGLRenderer2D> Create() { return std::make_shared<OpenGLRenderer2D>(); }
    private:
        void InitBatch();
        void FlushBatch();
        void OnResize();
        void OnUpdate();
        void UpdateViewportSize();
    private:
        struct UniformBufferData { glm::mat4 ViewProjectionMatrix; };
        struct Batch
        {
            uint32_t VertexBufferId, IndexBufferId, VertexArrayId, ShaderProgramId;
            std::vector<uint32_t> TextureIds;
            std::vector<OpenGLVertex> Vertices;
        };
    private:
        OpenGLRenderer2DInitInfo m_RendererInitInfo;
        uint32_t m_UniformBufferId;
        UniformBufferData m_UniformBufferData;
        Batch m_Batch;
        glm::vec2 m_ViewportSize;
        std::unordered_map<std::string, GLint> m_UniformLocationCache;
        GLFWwindow* m_UserWindow;

        float m_CurrentTextureSlot;

        const glm::vec4 m_TemplateVertexPositions[4] = {
             {-0.5f, -0.5f, 0.0f, 1.0f},
             {-0.5f,  0.5f, 0.0f, 1.0f},
             { 0.5f,  0.5f, 0.0f, 1.0f},
             { 0.5f, -0.5f, 0.0f, 1.0f}
        };

        const glm::vec4 m_DefaultColor = glm::vec4(1);
    };
}
