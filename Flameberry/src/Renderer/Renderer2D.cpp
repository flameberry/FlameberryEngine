#include "Renderer2D.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "RenderCommand.h"

namespace Flameberry {
    std::unordered_map<char, Renderer2D::Character> Renderer2D::s_Characters;
    Renderer2DInitInfo                        Renderer2D::s_RendererInitInfo;
    std::unordered_map<std::string, GLint>    Renderer2D::s_UniformLocationCache;
    std::unordered_map<std::string, uint32_t> Renderer2D::s_TextureIdCache;
    Renderer2D::Batch                         Renderer2D::s_Batch;
    uint32_t                                  Renderer2D::s_UniformBufferId;
    glm::vec2                                 Renderer2D::s_WindowContentScale;
    float                                     Renderer2D::s_AspectRatio = (float)(1280.0f / 720.0f);
    std::string                               Renderer2D::s_UserFontFilePath = "";
    Renderer2D::UniformBufferData             Renderer2D::s_UniformBufferData;
    Renderer2D::FontProps                     Renderer2D::s_FontProps = { .Scale = 1.0f, .Strength = 0.5f, .PixelRange = 8.0f };
    glm::vec2                                 Renderer2D::s_ViewportSize = { 1280.0f, 720.0f };
    glm::vec2                                 Renderer2D::s_CursorPosition = { 0.0f, 0.0f };
    float                                     Renderer2D::s_CurrentTextureSlot = 0;
    GLFWwindow* Renderer2D::s_UserWindow;

    void Renderer2D::UpdateViewportSize()
    {
        if (s_RendererInitInfo.enableCustomViewport)
            s_ViewportSize = s_RendererInitInfo.customViewportSize;
        else
        {
            int width, height;
            glfwGetFramebufferSize(s_UserWindow, &width, &height);
            s_ViewportSize = { width, height };
        }
    }

    void Renderer2D::UpdateWindowContentScale()
    {
        if (s_RendererInitInfo.enableCustomViewport)
            s_WindowContentScale = { 1.0f, 1.0f };
        else
        {
            glm::vec2 scale;
            glfwGetWindowContentScale(s_UserWindow, &scale.x, &scale.y);
            s_WindowContentScale = scale;
        }
    }

    void Renderer2D::OnResize()
    {
        UpdateViewportSize();
        s_AspectRatio = s_ViewportSize.x / s_ViewportSize.y;
        glViewport(0, 0, s_ViewportSize.x, s_ViewportSize.y);
    }

    glm::vec2  Renderer2D::GetViewportSize() { return s_ViewportSize; }
    glm::vec2& Renderer2D::GetCursorPosition() { return s_CursorPosition; }
    GLFWwindow* Renderer2D::GetUserGLFWwindow() { return s_UserWindow; }

    void Renderer2D::OnUpdate()
    {
        glClear(GL_DEPTH_BUFFER_BIT);

        UpdateWindowContentScale();
        OnResize();

        double x, y;
        glfwGetCursorPos(s_UserWindow, &x, &y);
        s_CursorPosition.x = x - s_ViewportSize.x / s_WindowContentScale.x / 2.0f;
        s_CursorPosition.y = -y + s_ViewportSize.y / s_WindowContentScale.y / 2.0f;
    }

    void Renderer2D::Init(const Renderer2DInitInfo& rendererInitInfo)
    {
        FL_LOGGER_INIT("FLAMEBERRY");
        FL_INFO("Initialized Logger!");

        s_RendererInitInfo = rendererInitInfo;
        s_UserWindow = rendererInitInfo.userWindow;

        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        const char* monitorName = glfwGetMonitorName(primaryMonitor);
        FL_INFO("Primary Monitor: {0}", monitorName);

        UpdateWindowContentScale();
        UpdateViewportSize();

        if (rendererInitInfo.enableFontRendering)
        {
            s_UserFontFilePath = rendererInitInfo.fontFilePath;
            // Load Font Here
            FL_INFO("Loaded Font from path \"{0}\"", s_UserFontFilePath);
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);

        /* Create Uniform Buffer */
        glGenBuffers(1, &s_UniformBufferId);
        glBindBuffer(GL_UNIFORM_BUFFER, s_UniformBufferId);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBufferData), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferRange(GL_UNIFORM_BUFFER, 0, s_UniformBufferId, 0, sizeof(UniformBufferData));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        InitBatch();
        FL_INFO("Initialized Renderer2D!");
    }

    void Renderer2D::InitBatch()
    {
        s_Batch.TextureIds.reserve(MAX_TEXTURE_SLOTS);

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

        glGenVertexArrays(1, &s_Batch.VertexArrayId);
        glBindVertexArray(s_Batch.VertexArrayId);

        glGenBuffers(1, &s_Batch.VertexBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, s_Batch.VertexBufferId);
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex2D), nullptr, GL_DYNAMIC_DRAW);

        glBindVertexArray(s_Batch.VertexArrayId);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)offsetof(Vertex2D, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)offsetof(Vertex2D, color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)offsetof(Vertex2D, texture_uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)offsetof(Vertex2D, texture_index));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)offsetof(Vertex2D, quad_dimensions));

        glGenBuffers(1, &s_Batch.IndexBufferId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Batch.IndexBufferId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glBindVertexArray(s_Batch.VertexArrayId);

        s_Batch.ShaderProgramId = RenderCommand::CreateShader(FL_PROJECT_DIR"Flameberry/assets/shaders/Quad.glsl");
        glUseProgram(s_Batch.ShaderProgramId);

        int samplers[MAX_TEXTURE_SLOTS];
        for (uint32_t i = 0; i < MAX_TEXTURE_SLOTS; i++)
            samplers[i] = i;
        glUniform1iv(Renderer2D::GetUniformLocation("u_TextureSamplers", s_Batch.ShaderProgramId), MAX_TEXTURE_SLOTS, samplers);
        glUseProgram(0);
    }

    void Renderer2D::FlushBatch()
    {
        if (!s_Batch.Vertices.size())
            return;

        glBindBuffer(GL_ARRAY_BUFFER, s_Batch.VertexBufferId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, s_Batch.Vertices.size() * sizeof(Vertex2D), s_Batch.Vertices.data());

        for (uint8_t i = 0; i < s_Batch.TextureIds.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, s_Batch.TextureIds[i]);
        }

        glUseProgram(s_Batch.ShaderProgramId);
        glBindVertexArray(s_Batch.VertexArrayId);
        glDrawElements(GL_TRIANGLES, (s_Batch.Vertices.size() / 4) * 6, GL_UNSIGNED_INT, 0);

        s_Batch.Vertices.clear();
        s_Batch.TextureIds.clear();
    }

    void Renderer2D::AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const glm::vec4& color, UnitType unitType)
    {
        Vertex2D vertices[4];

        vertices[0].texture_uv = { 0.0f, 0.0f };
        vertices[1].texture_uv = { 0.0f, 1.0f };
        vertices[2].texture_uv = { 1.0f, 1.0f };
        vertices[3].texture_uv = { 1.0f, 0.0f };

        glm::mat4 transformation{ 1.0f };

        switch (unitType)
        {
        case UnitType::PIXEL_UNITS:
            transformation = glm::translate(glm::mat4(1.0f), { RenderCommand::PixelToOpenGL({ position.x, position.y }, s_ViewportSize), position.z });
            transformation = glm::scale(transformation, { RenderCommand::PixelToOpenGL(dimensions, s_ViewportSize), 0.0f });
            break;
        case UnitType::OPENGL_UNITS:
            transformation = glm::translate(glm::mat4(1.0f), position);
            transformation = glm::scale(transformation, { dimensions, 0.0f });
            break;
        default:
            break;
        }

        for (uint8_t i = 0; i < 4; i++)
        {
            vertices[i].position = transformation * s_TemplateVertexPositions[i];
            vertices[i].color = color;
            vertices[i].quad_dimensions = RenderCommand::PixelToOpenGL(dimensions, s_ViewportSize);
        }

        for (uint8_t i = 0; i < 4; i++)
            s_Batch.Vertices.push_back(vertices[i]);
    }

    void Renderer2D::AddQuad(const glm::vec3& position, const glm::vec2& dimensions, const glm::vec4& color, const char* textureFilePath, UnitType unitType)
    {
        Vertex2D vertices[4];
        vertices[0].texture_uv = { 0.0f, 0.0f };
        vertices[1].texture_uv = { 0.0f, 1.0f };
        vertices[2].texture_uv = { 1.0f, 1.0f };
        vertices[3].texture_uv = { 1.0f, 0.0f };

        glm::mat4 transformation{ 1.0f };

        switch (unitType)
        {
        case UnitType::PIXEL_UNITS:
            transformation = glm::translate(glm::mat4(1.0f), { RenderCommand::PixelToOpenGL({ position.x, position.y }, s_ViewportSize), position.z });
            transformation = glm::scale(transformation, { RenderCommand::PixelToOpenGL(dimensions, s_ViewportSize), 0.0f });
            break;
        case UnitType::OPENGL_UNITS:
            transformation = glm::translate(glm::mat4(1.0f), position);
            transformation = glm::scale(transformation, { dimensions, 0.0f });
            break;
        default:
            break;
        }

        for (uint8_t i = 0; i < 4; i++)
        {
            vertices[i].position = transformation * s_TemplateVertexPositions[i];
            vertices[i].color = color;
            vertices[i].quad_dimensions = RenderCommand::PixelToOpenGL(dimensions, s_ViewportSize);
            vertices[i].texture_index = s_CurrentTextureSlot;
        }

        for (uint8_t i = 0; i < 4; i++)
            s_Batch.Vertices.push_back(vertices[i]);

        uint32_t textureId = GetTextureIdIfAvailable(textureFilePath);
        if (!textureId)
        {
            textureId = RenderCommand::CreateTexture(textureFilePath);
            s_TextureIdCache[textureFilePath] = textureId;
        }
        s_Batch.TextureIds.push_back(textureId);

        // Increment the texture slot every time a textured quad is added
        s_CurrentTextureSlot++;
        if (s_CurrentTextureSlot == MAX_TEXTURE_SLOTS)
        {
            FlushBatch();
            s_CurrentTextureSlot = 0;
        }
    }

    void Renderer2D::AddText(const std::string& text, const glm::vec2& position_in_pixels, float scale, const glm::vec4& color)
    {
        static uint16_t slot = 0;
        auto position = position_in_pixels;

        for (std::string::const_iterator it = text.begin(); it != text.end(); it++)
        {
            Character character = s_Characters[*it];

            float xpos = position.x + character.bearing.x * scale;
            float ypos = position.y - (character.size.y - character.bearing.y) * scale - s_FontProps.DescenderY * scale;

            float w = character.size.x * scale;
            float h = character.size.y * scale;

            std::array<Flameberry::Vertex2D, 4> vertices;
            vertices[0].position = { xpos,     ypos,     0.0f };
            vertices[1].position = { xpos,     ypos + h, 0.0f };
            vertices[2].position = { xpos + w, ypos + h, 0.0f };
            vertices[3].position = { xpos + w, ypos,     0.0f };

            for (auto& vertex : vertices)
            {
                vertex.position.x = ConvertXAxisPixelValueToOpenGLValue(vertex.position.x);
                vertex.position.y = ConvertYAxisPixelValueToOpenGLValue(vertex.position.y);
            }

            vertices[0].texture_uv = { 0.0f, 0.0f };
            vertices[1].texture_uv = { 0.0f, 1.0f };
            vertices[2].texture_uv = { 1.0f, 1.0f };
            vertices[3].texture_uv = { 1.0f, 0.0f };

            for (auto& vertex : vertices)
                vertex.texture_index = (float)slot;

            slot++;
            if (slot == MAX_TEXTURE_SLOTS)
                slot = 0;

            for (uint8_t i = 0; i < 4; i++)
                vertices[i].color = color;

            position.x += (character.advance) * scale + 1.0f;
        }
    }

    void Renderer2D::Begin(OrthographicCamera& camera)
    {
        OnUpdate();
        camera.OnUpdate();
        s_UniformBufferData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();

        /* Set Projection Matrix in GPU memory, for all shader programs to access it */
        glBindBuffer(GL_UNIFORM_BUFFER, s_UniformBufferId);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(s_UniformBufferData.ViewProjectionMatrix));
    }

    void Renderer2D::End()
    {
        FlushBatch();
        s_CurrentTextureSlot = 0;
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    glm::vec2 Renderer2D::ConvertPixelsToOpenGLValues(const glm::vec2& value_in_pixels)
    {
        glm::vec2 position_in_opengl_coords;
        position_in_opengl_coords.x = value_in_pixels.x * ((2.0f * s_AspectRatio) / s_ViewportSize.x) * s_WindowContentScale.x;
        position_in_opengl_coords.y = value_in_pixels.y * (2.0f / s_ViewportSize.y) * s_WindowContentScale.y;
        return position_in_opengl_coords;
    }

    glm::vec2 Renderer2D::ConvertOpenGLValuesToPixels(const glm::vec2& opengl_coords)
    {
        glm::vec2 value_in_pixels;
        int width, height;
        glfwGetFramebufferSize(s_UserWindow, &width, &height);
        value_in_pixels.x = (int)((opengl_coords.x / (2.0f * s_AspectRatio)) * width);
        value_in_pixels.y = (int)((opengl_coords.y / 2.0f) * height);
        return value_in_pixels;
    }

    float Renderer2D::ConvertXAxisPixelValueToOpenGLValue(int X)
    {
        return static_cast<float>(X) * ((2.0f * s_AspectRatio) / s_ViewportSize.x) * s_WindowContentScale.x;
    }

    float Renderer2D::ConvertYAxisPixelValueToOpenGLValue(int Y)
    {
        return static_cast<float>(Y) * (2.0f / s_ViewportSize.y) * s_WindowContentScale.y;
    }

    GLint Renderer2D::GetUniformLocation(const std::string& name, uint32_t shaderId)
    {
        if (s_UniformLocationCache.find(name) != s_UniformLocationCache.end())
            return s_UniformLocationCache[name];

        GLint location = glGetUniformLocation(shaderId, name.c_str());
        if (location == -1)
            FL_WARN("Uniform \"{0}\" not found!", name);
        s_UniformLocationCache[name] = location;
        return location;
    }

    uint32_t Renderer2D::GetTextureIdIfAvailable(const char* textureFilePath)
    {
        if (s_TextureIdCache.find(textureFilePath) != s_TextureIdCache.end())
            return s_TextureIdCache[textureFilePath];
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
