#include "OpenGLRenderer2D.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "OpenGLRenderCommand.h"
#include "OpenGLUniformBufferIndices.h"
#include "Core/Input.h"

namespace Flameberry {
    OpenGLRenderer2D::OpenGLRenderer2D()
        : m_CameraUniformBuffer(sizeof(UniformBufferData), nullptr, GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW), m_CurrentTextureSlot(0)
    {
        m_CameraUniformBuffer.BindBufferBase(FL_UNIFORM_BLOCK_BINDING_CAMERA_2D);
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
        m_Batch.TextureIds.reserve(MAX_TEXTURE_SLOTS);

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

        m_Batch.ShaderProgramId = OpenGLRenderCommand::CreateShader(FL_PROJECT_DIR"Flameberry/assets/shaders/Quad.glsl");
        glUseProgram(m_Batch.ShaderProgramId);

        int samplers[MAX_TEXTURE_SLOTS];
        for (uint32_t i = 0; i < MAX_TEXTURE_SLOTS; i++)
            samplers[i] = i;
        glUniform1iv(OpenGLRenderCommand::GetUniformLocation(m_Batch.ShaderProgramId, "u_TextureSamplers"), MAX_TEXTURE_SLOTS, samplers);
        glUseProgram(0);
    }

    void OpenGLRenderer2D::FlushBatch()
    {
        if (!m_Batch.Vertices.size())
            return;

        glBindBuffer(GL_ARRAY_BUFFER, m_Batch.VertexBufferId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_Batch.Vertices.size() * sizeof(OpenGLVertex2D), m_Batch.Vertices.data());

        for (uint8_t i = 0; i < m_Batch.TextureIds.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, m_Batch.TextureIds[i]);
        }

        glUseProgram(m_Batch.ShaderProgramId);
        glBindVertexArray(m_Batch.VertexArrayId);
        glDrawElements(GL_TRIANGLES, (m_Batch.Vertices.size() / 4) * 6, GL_UNSIGNED_INT, 0);

        m_Batch.Vertices.clear();
        m_Batch.TextureIds.clear();
        m_CurrentTextureSlot = 0;
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

    void OpenGLRenderer2D::AddQuad(const glm::mat4& transform, const char* textureFilePath, uint32_t entityID)
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

        uint32_t textureID = OpenGLRenderCommand::CreateTexture(textureFilePath);
        m_Batch.TextureIds.push_back(textureID);

        // Increment the texture slot every time a textured quad is added
        m_CurrentTextureSlot++;
        if (m_CurrentTextureSlot == MAX_TEXTURE_SLOTS)
        {
            FlushBatch();
            m_CurrentTextureSlot = 0;
        }
    }

    void OpenGLRenderer2D::AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const char* textureFilePath)
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

        uint32_t textureID = OpenGLRenderCommand::CreateTexture(textureFilePath);
        m_Batch.TextureIds.push_back(textureID);

        // Increment the texture slot every time a textured quad is added
        m_CurrentTextureSlot++;
        if (m_CurrentTextureSlot == MAX_TEXTURE_SLOTS)
        {
            FlushBatch();
            m_CurrentTextureSlot = 0;
        }
    }

    void OpenGLRenderer2D::Begin(const OrthographicCamera& camera)
    {
        m_UniformBufferData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();

        m_CameraUniformBuffer.Bind();
        m_CameraUniformBuffer.BufferSubData(&m_UniformBufferData, sizeof(UniformBufferData), 0);
    }

    void OpenGLRenderer2D::End()
    {
        FlushBatch();
        m_CameraUniformBuffer.Unbind();
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
