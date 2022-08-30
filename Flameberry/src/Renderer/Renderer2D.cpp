#include "Renderer2D.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "RenderCommand.h"
#include "Core/Input.h"

namespace Flameberry {
    Renderer2D::Renderer2D()
        : m_AspectRatio(1280.0f / 720.0f), m_ViewportSize(1280.0f, 720.0f), m_CursorPosition(0.0f, 0.0f), m_CurrentTextureSlot(0)
    {}

    Renderer2D::~Renderer2D()
    {}

    void Renderer2D::UpdateViewportSize()
    {
        if (m_RendererInitInfo.enableCustomViewport)
            m_ViewportSize = m_RendererInitInfo.customViewportSize;
        else
        {
            int width, height;
            glfwGetFramebufferSize(m_UserWindow, &width, &height);
            m_ViewportSize = { width, height };
        }
    }

    void Renderer2D::UpdateWindowContentScale()
    {
        if (m_RendererInitInfo.enableCustomViewport)
            m_WindowContentScale = { 1.0f, 1.0f };
        else
        {
            glm::vec2 scale;
            glfwGetWindowContentScale(m_UserWindow, &scale.x, &scale.y);
            m_WindowContentScale = scale;
        }
    }

    void Renderer2D::OnResize()
    {
        UpdateViewportSize();
        m_AspectRatio = m_ViewportSize.x / m_ViewportSize.y;
        glViewport(0, 0, m_ViewportSize.x, m_ViewportSize.y);
    }

    glm::vec2  Renderer2D::GetViewportSize() { return m_ViewportSize; }
    glm::vec2& Renderer2D::GetCursorPosition() { return m_CursorPosition; }
    GLFWwindow* Renderer2D::GetUserGLFWwindow() { return m_UserWindow; }

    void Renderer2D::OnUpdate()
    {
        glClear(GL_DEPTH_BUFFER_BIT);

        UpdateWindowContentScale();
        OnResize();

        double x, y;
        glfwGetCursorPos(m_UserWindow, &x, &y);
        m_CursorPosition.x = x - m_ViewportSize.x / m_WindowContentScale.x / 2.0f;
        m_CursorPosition.y = -y + m_ViewportSize.y / m_WindowContentScale.y / 2.0f;
    }

    void Renderer2D::Init(const Renderer2DInitInfo& rendererInitInfo)
    {
        m_RendererInitInfo = rendererInitInfo;
        m_UserWindow = rendererInitInfo.userWindow;

        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        const char* monitorName = glfwGetMonitorName(primaryMonitor);
        FL_INFO("Primary Monitor: {0}", monitorName);

        UpdateWindowContentScale();
        UpdateViewportSize();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);

        /* Create Uniform Buffer */
        glGenBuffers(1, &m_UniformBufferId);
        glBindBuffer(GL_UNIFORM_BUFFER, m_UniformBufferId);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBufferData), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferRange(GL_UNIFORM_BUFFER, 0, m_UniformBufferId, 0, sizeof(UniformBufferData));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        InitBatch();
        FL_INFO("Initialized Renderer2D!");
    }

    void Renderer2D::InitBatch()
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
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex2D), nullptr, GL_DYNAMIC_DRAW);

        glBindVertexArray(m_Batch.VertexArrayId);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)offsetof(Vertex2D, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)offsetof(Vertex2D, color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)offsetof(Vertex2D, texture_uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)offsetof(Vertex2D, texture_index));

        glGenBuffers(1, &m_Batch.IndexBufferId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Batch.IndexBufferId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glBindVertexArray(m_Batch.VertexArrayId);

        m_Batch.ShaderProgramId = RenderCommand::CreateShader(FL_PROJECT_DIR"Flameberry/assets/shaders/Quad.glsl");
        glUseProgram(m_Batch.ShaderProgramId);

        int samplers[MAX_TEXTURE_SLOTS];
        for (uint32_t i = 0; i < MAX_TEXTURE_SLOTS; i++)
            samplers[i] = i;
        glUniform1iv(Renderer2D::GetUniformLocation("u_TextureSamplers", m_Batch.ShaderProgramId), MAX_TEXTURE_SLOTS, samplers);
        glUseProgram(0);
    }

    void Renderer2D::FlushBatch()
    {
        if (!m_Batch.Vertices.size())
            return;

        glBindBuffer(GL_ARRAY_BUFFER, m_Batch.VertexBufferId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_Batch.Vertices.size() * sizeof(Vertex2D), m_Batch.Vertices.data());

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

    void Renderer2D::AddQuad(const glm::mat4& transform, const glm::vec4& color)
    {
        Vertex2D vertices[4];

        vertices[0].texture_uv = { 0.0f, 0.0f };
        vertices[1].texture_uv = { 0.0f, 1.0f };
        vertices[2].texture_uv = { 1.0f, 1.0f };
        vertices[3].texture_uv = { 1.0f, 0.0f };

        for (uint8_t i = 0; i < 4; i++)
        {
            vertices[i].position = transform * m_TemplateVertexPositions[i];
            vertices[i].color = color;
        }

        for (uint8_t i = 0; i < 4; i++)
            m_Batch.Vertices.push_back(vertices[i]);
    }

    void Renderer2D::AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const glm::vec4& color)
    {
        Vertex2D vertices[4];

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

    void Renderer2D::AddQuad(const glm::mat4& transform, const char* textureFilePath)
    {
        Vertex2D vertices[4];
        vertices[0].texture_uv = { 0.0f, 0.0f };
        vertices[1].texture_uv = { 0.0f, 1.0f };
        vertices[2].texture_uv = { 1.0f, 1.0f };
        vertices[3].texture_uv = { 1.0f, 0.0f };

        for (uint8_t i = 0; i < 4; i++)
        {
            vertices[i].position = transform * m_TemplateVertexPositions[i];
            vertices[i].color = m_DefaultColor;
            vertices[i].texture_index = m_CurrentTextureSlot;
        }

        for (uint8_t i = 0; i < 4; i++)
            m_Batch.Vertices.push_back(vertices[i]);

        uint32_t textureId = GetTextureIdIfAvailable(textureFilePath);
        if (!textureId)
        {
            textureId = RenderCommand::CreateTexture(textureFilePath);
            m_TextureIdCache[textureFilePath] = textureId;
        }
        m_Batch.TextureIds.push_back(textureId);

        // Increment the texture slot every time a textured quad is added
        m_CurrentTextureSlot++;
        if (m_CurrentTextureSlot == MAX_TEXTURE_SLOTS)
        {
            FlushBatch();
            m_CurrentTextureSlot = 0;
        }
    }

    void Renderer2D::AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const char* textureFilePath)
    {
        Vertex2D vertices[4];
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

        uint32_t textureId = GetTextureIdIfAvailable(textureFilePath);
        if (!textureId)
        {
            textureId = RenderCommand::CreateTexture(textureFilePath);
            m_TextureIdCache[textureFilePath] = textureId;
        }
        m_Batch.TextureIds.push_back(textureId);

        // Increment the texture slot every time a textured quad is added
        m_CurrentTextureSlot++;
        if (m_CurrentTextureSlot == MAX_TEXTURE_SLOTS)
        {
            FlushBatch();
            m_CurrentTextureSlot = 0;
        }
    }

    void Renderer2D::Begin(OrthographicCamera& camera)
    {
        OnUpdate();

        camera.OnUpdate();
        m_UniformBufferData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();

        /* Set Projection Matrix in GPU memory, for all shader programs to access it */
        glBindBuffer(GL_UNIFORM_BUFFER, m_UniformBufferId);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(m_UniformBufferData.ViewProjectionMatrix));
    }

    void Renderer2D::End()
    {
        FlushBatch();
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    GLint Renderer2D::GetUniformLocation(const std::string& name, uint32_t shaderId)
    {
        if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
            return m_UniformLocationCache[name];

        GLint location = glGetUniformLocation(shaderId, name.c_str());
        if (location == -1)
            FL_WARN("Uniform \"{0}\" not found!", name);
        m_UniformLocationCache[name] = location;
        return location;
    }

    uint32_t Renderer2D::GetTextureIdIfAvailable(const char* textureFilePath)
    {
        if (m_TextureIdCache.find(textureFilePath) != m_TextureIdCache.end())
            return m_TextureIdCache[textureFilePath];
        else
            return 0;
    }

    void Renderer2D::CleanUp()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
}
