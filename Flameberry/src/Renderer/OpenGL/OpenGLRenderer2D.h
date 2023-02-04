#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "OpenGLVertex.h"
#include "OpenGLBuffer.h"
#include "OpenGLShader.h"

#include "Core/Core.h"
#include "Renderer/OrthographicCamera.h"

// This Macro contains the max number of texture slots that the GPU supports, varies for each computer.
#define MAX_TEXTURE_SLOTS 16
#define MAX_QUADS 1000
#define MAX_VERTICES 4 * MAX_QUADS
#define MAX_INDICES 6 * MAX_QUADS

namespace Flameberry {
    class OpenGLRenderer2D
    {
    public:
        OpenGLRenderer2D();
        ~OpenGLRenderer2D();

        void Init();
        void AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const char* textureFilePath);
        void AddQuad(const glm::mat4& transform, const glm::vec4& color, uint32_t entityID);
        void AddQuad(const glm::mat4& transform, const char* textureFilePath, uint32_t entityID);
        void AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const glm::vec4& color);
        void CleanUp();
        void Begin(const OrthographicCamera& camera);
        void Begin(const glm::mat4& cameraMatrix);
        void End();
        static std::shared_ptr<OpenGLRenderer2D> Create() { return std::make_shared<OpenGLRenderer2D>(); }
    private:
        void InitBatch();
        void FlushBatch();
    private:
        struct UniformBufferData { glm::mat4 ViewProjectionMatrix; };
        struct Batch
        {
            uint32_t VertexBufferId, IndexBufferId, VertexArrayId;
            std::vector<uint32_t> TextureIds;
            std::vector<OpenGLVertex2D> Vertices;
        };
    private:
        uint32_t m_UniformBufferId;
        UniformBufferData m_UniformBufferData;
        Batch m_Batch;

        OpenGLBuffer m_CameraUniformBuffer;
        std::shared_ptr<OpenGLShader> m_QuadShader;

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
