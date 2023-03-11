#include "OpenGLRenderer2D.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "OpenGLRenderCommand.h"
#include "OpenGLUniformBufferIndices.h"
#include "Core/Input.h"
#include "Renderer/PerspectiveCamera.h"

namespace Flameberry {
    OpenGLRenderer2D::OpenGLRenderer2D()
        : m_CameraUniformBuffer(sizeof(UniformBufferData), nullptr, GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW),
        m_CurrentTextureSlot(0)
    {
        m_CameraUniformBuffer.BindBufferBase(FL_UNIFORM_BLOCK_BINDING_CAMERA_2D);

        OpenGLShaderBinding cameraBinding{};
        cameraBinding.blockName = "Camera";
        cameraBinding.blockBindingIndex = FL_UNIFORM_BLOCK_BINDING_CAMERA;

        m_QuadShader = OpenGLShader::Create(FL_PROJECT_DIR"Flameberry/assets/shaders/Quad.glsl", { cameraBinding });
        m_QuadShader->Bind();
        int samplers[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        m_QuadShader->PushUniformIntArray("u_TextureSamplers", 16, samplers);
        m_QuadShader->Unbind();
    }

    OpenGLRenderer2D::~OpenGLRenderer2D()
    {
    }

    void OpenGLRenderer2D::Init()
    {
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        const char* monitorName = glfwGetMonitorName(primaryMonitor);
        FL_INFO("Primary Monitor: {0}", monitorName);

        InitBatch();
        FL_INFO("Initialized OpenGL Renderer2D!");
    }

    void OpenGLRenderer2D::InitBatch()
    {
        m_Batch.Textures.reserve(MAX_TEXTURE_SLOTS);

        uint32_t indices[MAX_INDICES];
        size_t offset = 0;
        for (size_t i = 0; i < MAX_INDICES; i += 6)
        {
            indices[0 + i] = 1 + offset;
            indices[1 + i] = 2 + offset;
            indices[2 + i] = 3 + offset;

            indices[3 + i] = 3 + offset;
            indices[4 + i] = 0 + offset;
            indices[5 + i] = 1 + offset;

            offset += 4;
        }

        glGenVertexArrays(1, &m_Batch.VertexArrayId);
        glBindVertexArray(m_Batch.VertexArrayId);

        glGenBuffers(1, &m_Batch.VertexBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, m_Batch.VertexBufferId);
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(OpenGLVertex2D), nullptr, GL_DYNAMIC_DRAW);

        glBindVertexArray(m_Batch.VertexArrayId);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float) + sizeof(int), (void*)offsetof(OpenGLVertex2D, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float) + sizeof(int), (void*)offsetof(OpenGLVertex2D, color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float) + sizeof(int), (void*)offsetof(OpenGLVertex2D, texture_uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float) + sizeof(int), (void*)offsetof(OpenGLVertex2D, texture_index));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 1, GL_INT, GL_FALSE, 10 * sizeof(float) + sizeof(int), (void*)offsetof(OpenGLVertex2D, entityID));

        glGenBuffers(1, &m_Batch.IndexBufferId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Batch.IndexBufferId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glBindVertexArray(m_Batch.VertexArrayId);
    }

    void OpenGLRenderer2D::FlushBatch(const std::shared_ptr<OpenGLShader>& shader)
    {
        if (!m_Batch.Vertices.size())
            return;

        glBindBuffer(GL_ARRAY_BUFFER, m_Batch.VertexBufferId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_Batch.Vertices.size() * sizeof(OpenGLVertex2D), m_Batch.Vertices.data());

        for (uint8_t i = 0; i < m_Batch.Textures.size(); i++)
            m_Batch.Textures[i]->BindTextureUnit(i);

        shader->Bind();
        glBindVertexArray(m_Batch.VertexArrayId);
        glDrawElements(GL_TRIANGLES, (m_Batch.Vertices.size() / 4) * 6, GL_UNSIGNED_INT, 0);

        m_Batch.Vertices.clear();
        m_Batch.Textures.clear();
        m_CurrentTextureSlot = 0;
    }

    void OpenGLRenderer2D::AddQuad(const OpenGLVertex2D* vertices, int count)
    {
        for (uint16_t i = 0; i < count; i++)
            m_Batch.Vertices.push_back(vertices[i]);
    }

    void OpenGLRenderer2D::AddQuad(const OpenGLVertex2D* vertices, int count, const std::shared_ptr<OpenGLTexture>& texture)
    {
        for (uint16_t i = 0; i < count; i++)
            m_Batch.Vertices.push_back(vertices[i]);

        // uint32_t textureID = OpenGLRenderCommand::CreateTexture(texturePath);
        // m_Batch.Textures.push_back(textureID);
        m_Batch.Textures.push_back(texture);

        // Increment the texture slot every time a textured quad is added
        m_CurrentTextureSlot++;
        if (m_CurrentTextureSlot == MAX_TEXTURE_SLOTS)
        {
            FlushBatch(m_QuadShader);
            m_CurrentTextureSlot = 0;
        }
    }

    void OpenGLRenderer2D::AddBillBoard(const glm::vec3& position, float size, const std::shared_ptr<OpenGLTexture>& texture, const glm::mat4& cameraViewMatrix)
    {
        const glm::vec2 m_GenericVertexOffsets[] = {
            {-1.0f, -1.0f},
            {-1.0f,  1.0f},
            { 1.0f,  1.0f},
            { 1.0f, -1.0f}
        };

        const glm::vec3& right = { cameraViewMatrix[0][0], cameraViewMatrix[1][0], cameraViewMatrix[2][0] };
        const glm::vec3& up = { cameraViewMatrix[0][1], cameraViewMatrix[1][1], cameraViewMatrix[2][1] };

        OpenGLVertex2D vertices[4];
        vertices[0].texture_uv = { 0.0f, 0.0f };
        vertices[1].texture_uv = { 0.0f, 1.0f };
        vertices[2].texture_uv = { 1.0f, 1.0f };
        vertices[3].texture_uv = { 1.0f, 0.0f };

        for (uint8_t i = 0; i < 4; i++)
        {
            vertices[i].position = position
                - size * m_GenericVertexOffsets[i].x * right
                + size * m_GenericVertexOffsets[i].y * up;
            vertices[i].color = m_DefaultColor;
            vertices[i].texture_index = m_CurrentTextureSlot;
        }

        for (uint8_t i = 0; i < 4; i++)
            m_Batch.Vertices.push_back(vertices[i]);

        // uint32_t textureID = OpenGLRenderCommand::CreateTexture(texturePath);
        m_Batch.Textures.push_back(texture);

        // Increment the texture slot every time a textured quad is added
        m_CurrentTextureSlot++;
        if (m_CurrentTextureSlot == MAX_TEXTURE_SLOTS)
        {
            FlushBatch(m_QuadShader);
            m_CurrentTextureSlot = 0;
        }
    }

    void OpenGLRenderer2D::AddBillBoardForMousePicking(const glm::vec3& position, float size, const glm::mat4& cameraViewMatrix, uint32_t entityID)
    {
        const glm::vec2 genericVertexOffsets[] = {
            {-1.0f, -1.0f},
            {-1.0f,  1.0f},
            { 1.0f,  1.0f},
            { 1.0f, -1.0f}
        };

        const glm::vec3& right = { cameraViewMatrix[0][0], cameraViewMatrix[1][0], cameraViewMatrix[2][0] };
        const glm::vec3& up = { cameraViewMatrix[0][1], cameraViewMatrix[1][1], cameraViewMatrix[2][1] };

        OpenGLVertex2D vertices[4];
        for (uint8_t i = 0; i < 4; i++)
        {
            vertices[i].position = position
                - size * genericVertexOffsets[i].x * right
                + size * genericVertexOffsets[i].y * up;
            vertices[i].entityID = entityID;
        }

        for (uint8_t i = 0; i < 4; i++)
            m_Batch.Vertices.push_back(vertices[i]);
    }

    void OpenGLRenderer2D::AddQuad(const glm::mat4& transform, const glm::vec4& color, uint32_t entityID)
    {
        OpenGLVertex2D vertices[4];

        vertices[0].texture_uv = { 0.0f, 0.0f };
        vertices[1].texture_uv = { 0.0f, 1.0f };
        vertices[2].texture_uv = { 1.0f, 1.0f };
        vertices[3].texture_uv = { 1.0f, 0.0f };

        for (uint8_t i = 0; i < 4; i++)
        {
            vertices[i].position = transform * m_TemplateVertexPositions[i];
            vertices[i].color = color;
            vertices[i].entityID = (int)entityID;
        }

        for (uint8_t i = 0; i < 4; i++)
            m_Batch.Vertices.push_back(vertices[i]);
    }

    void OpenGLRenderer2D::AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const glm::vec4& color)
    {
        OpenGLVertex2D vertices[4];

        vertices[0].texture_uv = { 0.0f, 0.0f };
        vertices[1].texture_uv = { 0.0f, 1.0f };
        vertices[2].texture_uv = { 1.0f, 1.0f };
        vertices[3].texture_uv = { 1.0f, 0.0f };

        glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position);
        transformation = glm::scale(transformation, { dimensions, 0.0f });

        for (uint8_t i = 0; i < 4; i++)
        {
            vertices[i].position = transformation * m_TemplateVertexPositions[i];
            vertices[i].color = color;
        }

        for (uint8_t i = 0; i < 4; i++)
            m_Batch.Vertices.push_back(vertices[i]);
    }

    void OpenGLRenderer2D::AddQuad(const glm::mat4& transform, const std::shared_ptr<OpenGLTexture>& texture, uint32_t entityID)
    {
        OpenGLVertex2D vertices[4];
        vertices[0].texture_uv = { 0.0f, 0.0f };
        vertices[1].texture_uv = { 0.0f, 1.0f };
        vertices[2].texture_uv = { 1.0f, 1.0f };
        vertices[3].texture_uv = { 1.0f, 0.0f };

        for (uint8_t i = 0; i < 4; i++)
        {
            vertices[i].position = transform * m_TemplateVertexPositions[i];
            vertices[i].color = m_DefaultColor;
            vertices[i].texture_index = m_CurrentTextureSlot;
            vertices[i].entityID = (int)entityID;
        }

        for (uint8_t i = 0; i < 4; i++)
            m_Batch.Vertices.push_back(vertices[i]);

        // uint32_t textureID = OpenGLRenderCommand::CreateTexture(textureFilePath);
        m_Batch.Textures.push_back(texture);

        // Increment the texture slot every time a textured quad is added
        m_CurrentTextureSlot++;
        if (m_CurrentTextureSlot == MAX_TEXTURE_SLOTS)
        {
            FlushBatch(m_QuadShader);
            m_CurrentTextureSlot = 0;
        }
    }

    void OpenGLRenderer2D::AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const std::shared_ptr<OpenGLTexture>& texture)
    {
        OpenGLVertex2D vertices[4];
        vertices[0].texture_uv = { 0.0f, 0.0f };
        vertices[1].texture_uv = { 0.0f, 1.0f };
        vertices[2].texture_uv = { 1.0f, 1.0f };
        vertices[3].texture_uv = { 1.0f, 0.0f };

        glm::mat4 transformation = glm::translate(glm::mat4(1.0f), position);
        transformation = glm::scale(transformation, { dimensions, 0.0f });

        for (uint8_t i = 0; i < 4; i++)
        {
            vertices[i].position = transformation * m_TemplateVertexPositions[i];
            vertices[i].color = m_DefaultColor;
            vertices[i].texture_index = m_CurrentTextureSlot;
        }

        for (uint8_t i = 0; i < 4; i++)
            m_Batch.Vertices.push_back(vertices[i]);

        // uint32_t textureID = OpenGLRenderCommand::CreateTexture(textureFilePath);
        m_Batch.Textures.push_back(texture);

        // Increment the texture slot every time a textured quad is added
        m_CurrentTextureSlot++;
        if (m_CurrentTextureSlot == MAX_TEXTURE_SLOTS)
        {
            FlushBatch(m_QuadShader);
            m_CurrentTextureSlot = 0;
        }
    }

    void OpenGLRenderer2D::Begin(const OrthographicCamera& camera)
    {
        m_UniformBufferData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();

        m_CameraUniformBuffer.Bind();
        m_CameraUniformBuffer.BufferSubData(&m_UniformBufferData, sizeof(UniformBufferData), 0);
    }

    void OpenGLRenderer2D::Begin(const glm::mat4& cameraMatrix)
    {
        m_UniformBufferData.ViewProjectionMatrix = cameraMatrix;

        m_CameraUniformBuffer.Bind();
        m_CameraUniformBuffer.BufferSubData(&m_UniformBufferData, sizeof(UniformBufferData), 0);
    }


    void OpenGLRenderer2D::End()
    {
        FlushBatch(m_QuadShader);
        // m_CameraUniformBuffer.Unbind();
    }

    void OpenGLRenderer2D::End(const std::shared_ptr<OpenGLShader>& shader)
    {
        FlushBatch(shader);
        // m_CameraUniformBuffer.Unbind();
    }

    void OpenGLRenderer2D::CleanUp()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
}
