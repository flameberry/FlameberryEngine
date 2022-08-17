#include "Renderer3D.h"
#include <glad/glad.h>
#include "RenderCommand.h"
#include <vector>
#include "Core/Core.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Flameberry {
    void Renderer3D::UpdateViewportSize()
    {
        int width, height;
        glfwGetFramebufferSize(m_UserGLFWwindow, &width, &height);
        m_ViewportSize = { (float)width, (float)height };
    }

    void Renderer3D::Begin(const PerspectiveCamera& camera)
    {
        static float rotation = 0.0f;
        static double prevTime = glfwGetTime();

        double crntTime = glfwGetTime();
        if (crntTime - prevTime >= 1 / 60)
        {
            rotation += 0.5f;
            prevTime = crntTime;
        }

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));

        UpdateViewportSize();
        m_AspectRatio = m_ViewportSize.x / m_ViewportSize.y;

        m_UniformBufferData.ModelViewProjectionMatrix = camera.GetViewProjectionMatrix() * model;

        /* Set Projection Matrix in GPU memory, for all shader programs to access it */
        glBindBuffer(GL_UNIFORM_BUFFER, m_UniformBufferId);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(m_UniformBufferData.ModelViewProjectionMatrix));
    }

    void Renderer3D::End()
    {
    }

    void Renderer3D::OnDraw()
    {
        std::vector<Vertex> vertices;
        Vertex v0;
        v0.position = { -0.5f, 0.0f,  0.5f };
        v0.color = { 0.83f, 0.70f, 0.44f, 1.0f };
        v0.texture_uv = { 0.0f, 0.0f };
        v0.texture_index = 0;
        Vertex v1;
        v1.position = { -0.5f, 0.0f, -0.5f };
        v1.color = { 0.83f, 0.70f, 0.44f, 1.0f };
        v1.texture_uv = { 1.0f, 0.0f };
        v1.texture_index = 0;
        Vertex v2;
        v2.position = { 0.5f, 0.0f, -0.5f };
        v2.color = { 0.83f, 0.70f, 0.44f, 1.0f };
        v2.texture_uv = { 0.0f, 0.0f };
        v2.texture_index = 0;
        Vertex v3;
        v3.position = { 0.5f, 0.0f,  0.5f };
        v3.color = { 0.83f, 0.70f, 0.44f, 1.0f };
        v3.texture_uv = { 1.0f, 0.0f };
        v3.texture_index = 0;
        Vertex v4;
        v4.position = { 0.0f, 0.8f,  0.0f };
        v4.color = { 0.92f, 0.86f, 0.76f, 1.0f };
        v4.texture_uv = { 0.5f, 1.0f };
        v4.texture_index = 0;

        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);
        vertices.push_back(v4);

        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());

        // for (uint8_t i = 0; i < s_Batch.TextureIds.size(); i++)
        // {
        //     glActiveTexture(GL_TEXTURE0 + i);
        //     glBindTexture(GL_TEXTURE_2D, s_Batch.TextureIds[i]);
        // }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_TextureId);

        glUseProgram(m_ShaderProgramId);
        glBindVertexArray(m_VertexArrayId);
        // glDrawElements(GL_TRIANGLES, (vertices.size() / 4) * 6, GL_UNSIGNED_INT, 0);
        glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void Renderer3D::Init(GLFWwindow* window)
    {
        FL_LOGGER_INIT("FLAMEBERRY");
        FL_INFO("Initialized Logger!");

        m_TextureId = RenderCommand::CreateTexture(FL_PROJECT_DIR"SandboxApp/assets/textures/brick.png");

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);

        /* Create Uniform Buffer */
        glGenBuffers(1, &m_UniformBufferId);
        glBindBuffer(GL_UNIFORM_BUFFER, m_UniformBufferId);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBufferData), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferRange(GL_UNIFORM_BUFFER, 0, m_UniformBufferId, 0, sizeof(UniformBufferData));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        m_UserGLFWwindow = window;
        UpdateViewportSize();

        glGenVertexArrays(1, &m_VertexArrayId);
        glBindVertexArray(m_VertexArrayId);

        glGenBuffers(1, &m_VertexBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferId);
        glBufferData(GL_ARRAY_BUFFER, 1000 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

        glBindVertexArray(m_VertexArrayId);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)offsetof(Vertex, texture_uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)offsetof(Vertex, texture_index));

        uint32_t indices[] =
        {
            0, 1, 2,
            0, 2, 3,
            0, 1, 4,
            1, 2, 4,
            2, 3, 4,
            3, 0, 4
        };

        glGenBuffers(1, &m_IndexBufferId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glBindVertexArray(m_VertexArrayId);

        m_ShaderProgramId = RenderCommand::CreateShader(FL_PROJECT_DIR"Flameberry/assets/shaders/Default.glsl");
        glUseProgram(m_ShaderProgramId);
        int samplers[16];
        for (uint32_t i = 0; i < 16; i++)
            samplers[i] = i;
        glUniform1iv(GetUniformLocation("u_TextureSamplers", m_ShaderProgramId), 16, samplers);
        glUseProgram(0);
    }

    GLint Renderer3D::GetUniformLocation(const std::string& name, uint32_t shaderId)
    {
        if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
            return m_UniformLocationCache[name];

        GLint location = glGetUniformLocation(shaderId, name.c_str());
        if (location == -1)
            FL_WARN("Uniform \"{0}\" not found!", name);
        m_UniformLocationCache[name] = location;
        return location;
    }

    void Renderer3D::CleanUp()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }

    std::shared_ptr<Renderer3D> Renderer3D::Create()
    {
        return std::make_shared<Renderer3D>();
    }
}